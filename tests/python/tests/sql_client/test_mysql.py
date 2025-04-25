import re

import pytest

from pytest_mysql.factories import mysql, mysql_proc
from python.lib.testcase import WebServerAutoTestCase
from python.lib.file_utils import search_k2_bin

"""
Force disable mysql_proc fixture for K2 tests. 
Because of issue in CI with mysql on shutdown 
"""
@pytest.fixture(scope="session")
def mysql_proc_wrapper(request):
    if search_k2_bin() is not None:
        pytest.skip("Skipping mysql_proc in K2 mode")
    yield request.getfixturevalue("mysql_proc")
    request.getfixturevalue("mysql_proc")


@pytest.mark.k2_skip_suite
class TestMysql(WebServerAutoTestCase):
    @classmethod
    def extra_class_setup(cls):
        cls.web_server.update_options({
            "--verbosity-mysql=2": True,
            "-t": 10,
        })

    @pytest.fixture(autouse=True)
    def _setup_mysql_db(self, mysql, mysql_proc_wrapper):
        mysql_proc = mysql_proc_wrapper
        cursor = mysql.cursor()
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
        cursor.fetchall()
        cursor.close()
        mysql.commit()

        self.mysql_client = mysql
        self.mysql_proc = mysql_proc

    def _sql_query_impl(self, query, expected_res, uri="/?name=mysql"):
        resp = self.web_server.http_post(
            uri=uri,
            json={
                "dbname": 'test',
                "host": '127.0.0.1',
                "port": self.mysql_proc.port,
                "user": self.mysql_proc.user,
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
                             expected_res={'error': ['42S02', 1146]})

    def test_fail_syntax_error(self):
        self._sql_query_impl(query='HELLO WORLD',
                             expected_res={'error': ['42000', 1064]})

    def test_resumable_friendly(self):
        self._sql_query_impl(query='SELECT sleep(0.5)',
                             expected_res=[{'0': '0', 'sleep(0.5)': '0'}],
                             uri="/resumable_test?name=mysql")
        server_log = self.web_server.get_log()
        pattern = "start_resumable_function(.|\n)*start_query(.|\n)*end_resumable_function(.|\n)*end_query"
        if not re.search(pattern, ''.join(server_log)):
            raise RuntimeError("cannot find match for pattern \"" + pattern + "\n")
