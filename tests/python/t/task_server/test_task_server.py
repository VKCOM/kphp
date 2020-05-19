import os
import hashlib
import random
from multiprocessing.dummy import Pool as ThreadPool

from python.lib.testcase import KphpServerAutoTestCase
from python.lib.engine import Engine
from python.lib.file_utils import search_engine_bin, replace_in_file, gen_random_aes_key_file


class TestTaskServer(KphpServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        aes_key_file = gen_random_aes_key_file(cls.kphp_server_working_dir)
        cls.task_engine = Engine(
            engine_bin=search_engine_bin("tasks-engine"),
            working_dir=cls.kphp_server_working_dir,
            options={"--verbosity-tasks=1": True, "--aes-pwd": aes_key_file, "--allow-loopback": True}
        )
        cls.task_engine.create_binlog()
        cls.task_engine.start()

        rpc_proxy_conf = os.path.join(cls.kphp_server_working_dir, "data/rpc-proxy.conf")
        replace_in_file(rpc_proxy_conf, "${PORT}", str(cls.task_engine.rpc_port))
        cls.rpc_proxy = Engine(
            engine_bin=search_engine_bin("rpc-proxy"),
            working_dir=cls.kphp_server_working_dir,
            options={"--aes-pwd": aes_key_file, "--allow-loopback": True, rpc_proxy_conf: True}
        )
        cls.rpc_proxy.start()

        lease_conf = os.path.join(cls.kphp_server_working_dir, "data/lease-worker-config.yaml")
        replace_in_file(lease_conf, "${PORT}", str(cls.rpc_proxy.rpc_port))
        cls.kphp_server.update_options({
            "--define": "task_engine_port={}".format(cls.task_engine.rpc_port),
            "--tasks-config": lease_conf,
            "--staging": True,
            "--force-net-encryption": True,
            "--aes-pwd": aes_key_file,
            "--http-port": None
        })

    @classmethod
    def extra_class_teardown(cls):
        cls.rpc_proxy.stop()
        cls.task_engine.stop()

    def _init_queues(self):
        self.task_engine.rpc_request(
            ("tasks.createQueueType", [
                ("type_name", "a"),
                ("settings", ("tasks.queueTypeSettings", [
                    ("fields_mask", 1 + 64 + 8 + (1 << 12)), ("is_enabled", True), ("default_retry_time", 1),
                    ("default_retry_num", 2), ("timelimit", 2), ("is_staging", True)
                ]))
            ]))
        self.task_engine.assert_log(
            expect=["Create queue type a"],
            message="Can't find creation queue type a message"
        )

        self.task_engine.rpc_request(
            ("tasks.createQueueType", [
                ("type_name", "b"),
                ("settings", ("tasks.queueTypeSettings", [
                    ("fields_mask", 1 + 64 + 8 + (1 << 12)), ("is_enabled", False), ("default_retry_time", 1),
                    ("default_retry_num", 2), ("timelimit", 2), ("is_staging", True)
                ]))
            ]))
        self.task_engine.assert_log(
            expect=["Create queue type b"],
            message="Can't find creation queue type b message"
        )

        self.task_engine.rpc_request(
            ("tasks.createQueueType", [
                ("type_name", "c"),
                ("settings", ("tasks.queueTypeSettings", [
                    ("fields_mask", 33 + 64 + 8 + (1 << 12)), ("is_enabled", True), ("default_retry_time", 1),
                    ("default_retry_num", 2), ("is_blocking", True), ("timelimit", 2), ("is_staging", True)
                ]))
            ]))
        self.task_engine.assert_log(
            expect=["Create queue type c"],
            message="Can't find creation queue type c message"
        )

    def _create_task(self, _):
        type_name = random.choice("abc")
        task_id = self.task_engine.rpc_request(
            ("tasks.addTask", [
                ("type_name", type_name),
                ("queue_id", ("vector", random.choice(([], [random.randint(0, 5)])))),
                ("task", ("tasks.task", [
                    ("fields_mask", 0), ("flags", 0), ("tag", ("vector", [])),
                    ("data", hashlib.md5(os.urandom(5)).hexdigest())
                ]))
            ]))
        return task_id, type_name

    def test_task_server(self):
        self._init_queues()
        initial_stats = self.kphp_server.get_stats("kphp_server.requests_total_")

        sent_types = []
        with ThreadPool() as pool:
            for task_id, type_name in pool.imap_unordered(self._create_task, range(300)):
                sent_types.append((task_id, type_name))

        sent_to_kphp = 0
        for task_id, type_name in sent_types:
            self.task_engine.assert_log(
                expect=[
                    "Generate new id = {}".format(task_id),
                    "Task {} .queue_type = {}. was created".format(task_id, type_name)
                ],
                message="Can't find creation task message"
            )

            if type_name != "b":
                sent_to_kphp += 1
                self.kphp_server.assert_log(
                    expect=["worked", "task type " + type_name],
                    message="Can't find worker log message"
                )
                self.task_engine.assert_log(
                    expect=[
                        "Task {} is in progress".format(task_id),
                        "Send 1 tasks to worker pid",
                        "Task id = {} was done".format(task_id),
                    ],
                    message="Can't find processing task message"
                )

        self.kphp_server.assert_stats(
            prefix="kphp_server.requests_total_",
            expected_added_stats={
                "incoming_queries": sent_to_kphp, "outgoing_queries": sent_to_kphp * 4
            },
            initial_stats=initial_stats,
            message="Can't wait request stats"
        )
        self.assertKphpNoTerminatedRequests()
