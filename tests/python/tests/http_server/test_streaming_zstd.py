import zstandard

from python.lib.testcase import KphpServerAutoTestCase

class TestStreamingZstd(KphpServerAutoTestCase):
    def test_streaming_zstd(self):
        cmp = zstandard.ZstdDecompressor()

        url = "/test_streaming_zstd"
        response = self.kphp_server.http_get(url, headers={
            "Host": "localhost",
            "Accept-Encoding": "zstd"
        })
        self.assertEqual(response.headers["Content-Encoding"], "zstd")
        stream_reader = cmp.stream_reader(response.content)
        uncompressed_body = stream_reader.read().decode('utf-8')
        stream_reader.close()
        self.assertEqual(uncompressed_body, """justo mea orcidignissim corrumpit iuvaret suscipit ocurreret donec turpis eget eros sonet habitasse vix reprimique litora porro nostra nec vidisse pretium facilis referrentur in fusce sententiae appareat quidam repudiandae doctus periculis hac quem suspendisse facilis habemus nibh nisi ex dignissim sociis cursus habitant habitant taciti auctor graeco no facilis persius erat venenatis mandamus graeci falli ipsum elit tation dico nam donec luctus alia vim mucius urna ceteros dicta ad eius postea sea atomorum tale eros comprehensam detracto scripserit non natoque ligula velit eleifend iaculis donec ridiculus ignota quaestio equidem appareat consequat eros percipit mei molestie appetere patrioque dicta mnesarchum<br />libris error cum mollis consectetur usu adversarium vulputate te tractatos gloriatur hinc massa an ponderum delenit persius partiendo elementum per nullam omnesque dolorum sodales lorem commodo magnis maecenas nobis consequat curabitur vidisse potenti eloquentiam risus pretium corrumpit postulant dictumst conclusionemque affert ceteros atqui mutat tantas splendide docendi dolor euismod placerat rhoncus sonet aliquid movet nonumes eius qui ei velit nisl mauris vocent doming maximus ignota definiebas has duo posuere repudiandae nobis ex prompta dicit conubia dolore sed neglegentur graeci ludus suscipiantur cetero suspendisse phasellus molestiae vitae labores ferri equidem autem corrumpit brute finibus tellus risus petentium partiendo epicuri aliquet hendrerit""")
