from python.lib.testcase import KphpServerAutoTestCase


class TestJobSharedMessages(KphpServerAutoTestCase):
    DEFAULT_DISTRIBUTION_STR = " \n 2 ,2, 2,\t 2, 1,\n  1, \n1,1 ,1  , 1 \n"

    def _do_test(self, kphp_options, reserved):
        self.kphp_server.update_options(kphp_options)
        self.kphp_server.restart()
        self.kphp_server.assert_stats(
            timeout=10,
            prefix="kphp_server.workers_job_memory_messages_",
            expected_added_stats={
                "shared_messages_buffers_reserved": reserved["messages"],
                "extra_buffers_256kb_buffers_reserved": reserved["256kb"],
                "extra_buffers_512kb_buffers_reserved": reserved["512kb"],
                "extra_buffers_1mb_buffers_reserved": reserved["1mb"],
                "extra_buffers_2mb_buffers_reserved": reserved["2mb"],
                "extra_buffers_4mb_buffers_reserved": reserved["4mb"],
                "extra_buffers_8mb_buffers_reserved": reserved["8mb"],
                "extra_buffers_16mb_buffers_reserved": reserved["16mb"],
                "extra_buffers_32mb_buffers_reserved": reserved["32mb"],
                "extra_buffers_64mb_buffers_reserved": reserved["64mb"],
            })

    def test_default_shared_memory_size_option(self):
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": self.DEFAULT_DISTRIBUTION_STR, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 36, "256kb": 18, "512kb": 10, "1mb": 4, "2mb": 1, "4mb": 1, "8mb": 1, "16mb": 0, "32mb": 0, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-memory-distribution-weights": self.DEFAULT_DISTRIBUTION_STR, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 92, "256kb": 46, "512kb": 22, "1mb": 12, "2mb": 3, "4mb": 1, "8mb": 1, "16mb": 1, "32mb": 0, "64mb": 0}
        )

    def test_custom_shared_memory_size_option(self):
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": self.DEFAULT_DISTRIBUTION_STR, "--job-workers-shared-memory-size": "120M"
            },
            reserved={"messages": 137, "256kb": 69, "512kb": 35, "1mb": 18, "2mb": 5, "4mb": 2, "8mb": 2, "16mb": 1, "32mb": 0, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-memory-distribution-weights": self.DEFAULT_DISTRIBUTION_STR, "--job-workers-shared-memory-size": "500M"
            },
            reserved={"messages": 571, "256kb": 286, "512kb": 142, "1mb": 72, "2mb": 17, "4mb": 9, "8mb": 4, "16mb": 3, "32mb": 2, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.2,
                "--job-workers-shared-memory-distribution-weights": self.DEFAULT_DISTRIBUTION_STR, "--job-workers-shared-memory-size": "7M"
            },
            reserved={"messages": 7, "256kb": 4, "512kb": 2, "1mb": 0, "2mb": 0, "4mb": 1, "8mb": 0, "16mb": 0, "32mb": 0, "64mb": 0}
        )

    def test_default_shared_memory_distribution_option(self):
        self._do_test(
            kphp_options={
                "--workers-num": 4, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": None, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 36, "256kb": 18, "512kb": 10, "1mb": 4, "2mb": 1, "4mb": 1, "8mb": 1, "16mb": 0,
                      "32mb": 0, "64mb": 0}
        )
        self._do_test(
            kphp_options={
                "--workers-num": 10, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": None, "--job-workers-shared-memory-size": None
            },
            reserved={"messages": 92, "256kb": 46, "512kb": 22, "1mb": 12, "2mb": 3, "4mb": 1, "8mb": 1, "16mb": 1, "32mb": 0, "64mb": 0}
        )

    def test_custom_shared_memory_distribution_option(self):
        self._do_test(
            kphp_options={
                "--workers-num": 2, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": "0.5,0.5,0.5,0.5,0.25,0.25, 0.25, 0.25, 0.25, 0.25",
                "--job-workers-shared-memory-size": "120M"
            },
            reserved={"messages": 137, "256kb": 69, "512kb": 35, "1mb": 18, "2mb": 5, "4mb": 2, "8mb": 2, "16mb": 1, "32mb": 0, "64mb": 0}
        )

        self._do_test(
            kphp_options={
                "--workers-num": 2, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": "1, 0, 0, 0, 0, 0, 0, 0, 0, 1",
                "--job-workers-shared-memory-size": "128M"
            },
            reserved={"messages": 511, "256kb": 0, "512kb": 0, "1mb": 0, "2mb": 0, "4mb": 0, "8mb": 0, "16mb": 0, "32mb": 0, "64mb": 1}
        )

        self._do_test(
            kphp_options={
                "--workers-num": 2, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": "10, 9, 8, 7, 6, 5, 4, 3, 2, 1",
                "--job-workers-shared-memory-size": "128M"
            },
            reserved={"messages": 186 + 1, "256kb": 83 + 1, "512kb": 37, "1mb": 16 + 1, "2mb": 6, "4mb": 2 + 1, "8mb": 1, "16mb": 0 + 1, "32mb": 0, "64mb": 0}
        )

        self._do_test(
            kphp_options={
                "--workers-num": 2, "--job-workers-ratio": 0.5,
                "--job-workers-shared-memory-distribution-weights": "1, 1, 1, 1, 1, 1, 1, 1, 1, 1",
                "--job-workers-shared-memory-size": "641M"
            },
            reserved={"messages": 512 + 1, "256kb": 256 + 1, "512kb": 128 + 1, "1mb": 64, "2mb": 32, "4mb": 16, "8mb": 8, "16mb": 4, "32mb": 2, "64mb": 1}
        )
