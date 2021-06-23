import json

from python.lib.testcase import KphpServerAutoTestCase


class TestMasterStats(KphpServerAutoTestCase):
    def test_smoke_mc_stats(self):
        mc_stats = self.kphp_server.http_get("/get_stats_by_mc?master-port={}".format(self.kphp_server.master_port))
        self.assertEqual(mc_stats.status_code, 200)
        self.assertNotEqual(mc_stats.text, "")
        stats_dict = {}
        for stat_line in mc_stats.text.splitlines():
            key, value = stat_line.split('\t', 1)
            stats_dict[key] = value

        self.assertIn("uptime", stats_dict)
        self.assertIn("kphp_version", stats_dict)
        self.assertIn("tot_queries", stats_dict)
        self.assertIn("recent_idle_percent", stats_dict)
        self.assertIn("cpu_usage(now,1m,10m,1h)", stats_dict)
        self.assertIn("running_workers_avg(1m,10m,1h)", stats_dict)

    def test_smoke_rpc_stats(self):
        mc_stats = self.kphp_server.http_get("/get_stats_by_rpc?master-port={}".format(self.kphp_server.master_port))
        self.assertEqual(mc_stats.status_code, 200)
        self.assertNotEqual(mc_stats.text, "")

        stats_dict = json.loads(mc_stats.text)
        self.assertIn("uptime", stats_dict)
        self.assertIn("version", stats_dict)
        self.assertIn("kphp_version", stats_dict)
        self.assertIn("vmrss_bytes", stats_dict)
        self.assertIn("tot_queries", stats_dict)
        self.assertIn("recent_idle_percent", stats_dict)
        self.assertIn("cpu_usage(now,1m,10m,1h)", stats_dict)
        self.assertIn("running_workers_avg(1m,10m,1h)", stats_dict)
