from python.lib.testcase import KphpServerAutoTestCase


class TestJobSharedMessages(KphpServerAutoTestCase):
    def _do_test(self, kphp_options, reserved):
        self.kphp_server.update_options(kphp_options)
        self.kphp_server.restart()
        self.kphp_server.assert_stats(
            prefix="kphp_server.workers_job_memory_messages_",
            expected_added_stats={
                "shared_messages_buffers_reserved": reserved["messages"],
                "extra_buffers_1mb_buffers_reserved": reserved["1mb"],
                "extra_buffers_2mb_buffers_reserved": reserved["2mb"],
                "extra_buffers_4mb_buffers_reserved": reserved["4mb"],
                "extra_buffers_8mb_buffers_reserved": reserved["8mb"],
                "extra_buffers_16mb_buffers_reserved": reserved["16mb"],
                "extra_buffers_32mb_buffers_reserved": reserved["32mb"],
                "extra_buffers_64mb_buffers_reserved": reserved["64mb"],
            })

    def test_default(self):
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-messages": None, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 8, "1mb": 4, "2mb": 2, "4mb": 1, "8mb": 2, "16mb": 0, "32mb": 0, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-messages": None, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 20, "1mb": 10, "2mb": 6, "4mb": 2, "8mb": 1, "16mb": 2, "32mb": 0, "64mb": 0}
        )

    def test_job_workers_shared_memory_size_option(self):
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-messages": None, "--job-workers-shared-memory-size": "120M"
            },
            reserved={"messages": 8, "1mb": 5, "2mb": 3, "4mb": 2, "8mb": 2, "16mb": 1, "32mb": 2, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-messages": None, "--job-workers-shared-memory-size": "500M"
            },
            reserved={"messages": 20, "1mb": 11, "2mb": 5, "4mb": 3, "8mb": 1, "16mb": 2, "32mb": 1, "64mb": 6}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-messages": None, "--job-workers-shared-memory-size": "7M"
            },
            reserved={"messages": 13, "1mb": 0, "2mb": 0, "4mb": 0, "8mb": 0, "16mb": 0, "32mb": 0, "64mb": 0}
        )

    def test_job_workers_shared_messages_option(self):
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-messages": 50, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 50, "1mb": 5, "2mb": 1, "4mb": 0, "8mb": 0, "16mb": 0, "32mb": 0, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-messages": 100, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 100, "1mb": 10, "2mb": 6, "4mb": 2, "8mb": 0, "16mb": 0, "32mb": 0, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-messages": 100, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 64, "1mb": 0, "2mb": 0, "4mb": 0, "8mb": 0, "16mb": 0, "32mb": 0, "64mb": 0}
        )

    def test_job_workers_shared_messages_and_memory_size_option(self):
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-messages": 50, "--job-workers-shared-memory-size": "120M"
            },
            reserved={"messages": 50, "1mb": 4, "2mb": 3, "4mb": 1, "8mb": 2, "16mb": 2, "32mb": 1, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-messages": 100, "--job-workers-shared-memory-size": "500M"
            },
            reserved={"messages": 100, "1mb": 11, "2mb": 5, "4mb": 3, "8mb": 2, "16mb": 1, "32mb": 2, "64mb": 5}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-messages": 150, "--job-workers-shared-memory-size": "50M"
            },
            reserved={"messages": 99, "1mb": 0, "2mb": 0, "4mb": 0, "8mb": 0, "16mb": 0, "32mb": 0, "64mb": 0}
        )
