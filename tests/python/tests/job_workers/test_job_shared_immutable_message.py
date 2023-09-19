from python.lib.testcase import KphpServerAutoTestCase


class TestJobSharedImmutableMessage(KphpServerAutoTestCase):
    JOB_PHP_ASSERT_ERROR = -105

    @classmethod
    def extra_class_setup(cls):
        cls.kphp_server.update_options({
            "--workers-num": 10,
            "--job-workers-ratio": 0.8,
            "--verbosity-job-workers=2": True,
            "--job-workers-shared-memory-size": "80M",
            "--job-workers-shared-memory-distribution-weights": '1,1,1,1,1,1,1,4,1,1',
        })

    def test_error_different_shared_memory_pieces(self):
        resp = self.kphp_server.http_post(
            uri="/test_error_different_shared_memory_pieces",
            json={
                "shared_data": (0, 100, 1),
                "batch_size": 10,
                "batch_count": 10,
            })
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {'job-ids': []})
        self.kphp_server.assert_log(["Warning: Can't extract common shared memory piece in multiple jobs request: requests\\[0\\] and requests\\[1\\] have different shared memory pieces"])

    def test_job_worker_start_multi_with_shared_data(self):
        self._test_job_worker_start_multi(uri='/test_job_worker_start_multi_with_shared_data',
                                          jobs_cnt=10, with_shared_data=True)

    def test_job_worker_start_multi_without_shared_data(self):
        self._test_job_worker_start_multi(uri='/test_job_worker_start_multi_without_shared_data',
                                          jobs_cnt=2, with_shared_data=False)

    def test_job_worker_start_multi_with_errors(self):
        self._test_job_worker_start_multi(uri='/test_job_worker_start_multi_with_errors',
                                          jobs_cnt=10, errors_cnt=5, with_shared_data=True)

    def _test_job_worker_start_multi(self, *, uri, with_shared_data, jobs_cnt, errors_cnt=0):
        stats_before = self.kphp_server.get_stats()

        start, end, step = 0, 2000000, 1
        cnt = (end - start) / step

        self.assertGreater(cnt * 8, 15 << 20)
        self.assertLess(cnt * 8, 16 << 20)
        # shared_data is array<int64_t> in kphp and its size is between 15MB and 16MB

        shared_data = [i for i in range(start, end, step)]
        batch_count = int(jobs_cnt)
        batch_size = len(shared_data) // batch_count
        error_indices = [2 * i for i in range(errors_cnt)]
        resp = self.kphp_server.http_post(
            uri=uri,
            json={
                "shared_data": (start, end, step),
                "batch_size": batch_size,
                "batch_count": batch_count,
                "error_indices": error_indices,
            })
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {"jobs-result": [
            {'range_sum': sum(shared_data[i * batch_size: i * batch_size + batch_size])} if i not in error_indices else
            {'error': 'php assert error\n', 'error_code': self.JOB_PHP_ASSERT_ERROR}
            for i in range(batch_count)]
        })

        if errors_cnt > 0:
            self.kphp_server.assert_log(errors_cnt * [
                'Warning: Critical error "Test php_assert" in file',
                "Critical error during script execution: php assert error"
            ])

        array_copies_cnt = 1 if with_shared_data else batch_count
        job_messages_cnt = 2 * batch_count + int(with_shared_data)
        self.kphp_server.assert_stats(
            timeout=10,
            initial_stats=stats_before,
            expected_added_stats={
                # ensure shared data is not leaked
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_acquired": job_messages_cnt,
                "kphp_server.workers_job_memory_messages_shared_messages_buffers_released": job_messages_cnt,
                "kphp_server.workers_job_memory_messages_shared_messages_buffer_acquire_fails": 0,
                # ensure we copy shared array only once when shared data is used:
                "kphp_server.workers_job_memory_messages_extra_buffers_16mb_buffers_acquired": array_copies_cnt,
                "kphp_server.workers_job_memory_messages_extra_buffers_16mb_buffer_acquire_fails": 0,
                "kphp_server.workers_job_memory_messages_extra_buffers_16mb_buffers_released": array_copies_cnt,
                "kphp_server.workers_job_memory_messages_extra_buffers_32mb_buffers_acquired": 0,
                "kphp_server.workers_job_memory_messages_extra_buffers_32mb_buffer_acquire_fails": 0,
                "kphp_server.workers_job_memory_messages_extra_buffers_64mb_buffers_acquired": 0,
                "kphp_server.workers_job_memory_messages_extra_buffers_64mb_buffer_acquire_fails": 0,
            })

    def _test_shared_memory_piece_copying_impl(self, label):
        for i in range(50):
            resp = self.kphp_server.http_post(uri="/test_shared_memory_piece_in_response", json={
                "label": label,
            })
            self.assertEqual(resp.status_code, 200)
            for ans in resp.json()['jobs-result']:
                self.assertEqual(ans, 42)

    def test_copy_instance_from_shared_memory_piece_to_response(self):
        self._test_shared_memory_piece_copying_impl("job_response:instance")

    def test_copy_array_from_shared_memory_piece_to_job_response(self):
        self._test_shared_memory_piece_copying_impl("job_response:array")

    def test_copy_string_from_shared_memory_piece_to_job_response(self):
        self._test_shared_memory_piece_copying_impl("job_response:string")

    def test_copy_mixed_from_shared_memory_piece_to_job_response(self):
        self._test_shared_memory_piece_copying_impl("job_response:mixed")

    def test_copy_instance_from_shared_memory_piece_to_instance_cache(self):
        self._test_shared_memory_piece_copying_impl("instance_cache:instance")

    def test_copy_instance_from_shared_memory_piece_to_job_request(self):
        self._test_shared_memory_piece_copying_impl("job_request:instance")


