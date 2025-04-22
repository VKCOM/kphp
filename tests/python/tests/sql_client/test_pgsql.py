import re

import pytest

import pytest_postgresql
from python.lib.testcase import WebServerAutoTestCase
from python.lib.file_utils import search_k2_bin

@pytest.mark.k2_skip_suite
class TestPgsql(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--verbosity-pgsql=2": True,
            "-t": 10,
        })

    @pytest.fixture(autouse=True)
    @pytest.mark.skipif(search_k2_bin() is not None, reason="DB tests are unsupported in K2")
    def _setup_pgsql_db(self, postgresql, postgresql_proc):
        cursor = postgresql.cursor()
        cursor.execute(
            '''
            CREATE TABLE TestTable
            (
                id INT NOT NULL,
                val_str VARCHAR(100) NOT NULL,
                val_float FLOAT,
                PRIMARY KEY (id)
            );

            INSERT INTO
                TestTable (id, val_str, val_float)
            VALUES
                (1, 'hello', '1.0'),
                (2, 'world', '4.0'),
                (3, 'a', '42.42');
            '''
        )
        postgresql.commit()
        cursor.close()

        self.pgsql_client = postgresql
        self.pgsql_proc = postgresql_proc

    def _sql_query_impl(self, query, expected_res, uri="/?name=pgsql"):
        resp = self.web_server.http_post(
            uri=uri,
            json={
                "dbname": self.pgsql_proc.dbname,
                "host": self.pgsql_proc.host,
                "port": self.pgsql_proc.port,
                "user": self.pgsql_proc.user,
                "query": query
            }
        )
        self.assertEqual(resp.status_code, 200)
        self.assertEqual(resp.json(), {'result': expected_res})

    def test_simple_read(self):
        self._sql_query_impl(query='SELECT val_float FROM TestTable WHERE id=3',
                             expected_res=[{'0': '42.42', 'val_float': '42.42'}])

    def test_simple_modify(self):
        self._sql_query_impl(query="INSERT INTO TestTable (id, val_str, val_float) "
                                   "VALUES (4, 'abacaba', '10.01'), (5, 'basda', '21.43');",
                             expected_res={"affected_rows": 2})
        self._sql_query_impl(query="SELECT val_str, val_float FROM TestTable WHERE id=5",
                             expected_res=[{'0': 'basda', '1': '21.43', 'val_str': 'basda', 'val_float': '21.43'}])
        self._sql_query_impl(query="UPDATE TestTable SET val_str = 'qqq' WHERE id=4", expected_res={"affected_rows": 1})
        self._sql_query_impl(query="DELETE from TestTable", expected_res={"affected_rows": 5})

    def test_fail_unexisted_table(self):
        self._sql_query_impl(query='SELECT * FROM UnexistedTable',
                             expected_res={'error': ['42P01', 7]})

    def test_fail_syntax_error(self):
        self._sql_query_impl(query='HELLO WORLD',
                             expected_res={'error': ['42601', 7]})

    def test_resumable_friendly(self):
        self._sql_query_impl(query='SELECT pg_sleep(0.5)',
                             expected_res=[{'0': '', 'pg_sleep': ''}],
                             uri="/resumable_test?name=pgsql")
        server_log = self.web_server.get_log()
        pattern = "start_resumable_function(.|\n)*start_query(.|\n)*end_resumable_function(.|\n)*end_query"
        if not re.search(pattern, ''.join(server_log)):
            raise RuntimeError("cannot find match for pattern \"" + pattern + "\n")

