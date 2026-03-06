from urllib.parse import urlencode

from python.lib.testcase import WebServerAutoTestCase


class TestMultipartContentType(WebServerAutoTestCase):

    def test_multipart_name_attributes(self):
        boundary = "------------------------d74496d66958873e"

        data = (f"--{boundary}\r\n"
                'Content-Disposition: form-data; name="name"\r\n'
                "\r\n"
                "Ivan\r\n"
                f"--{boundary}\r\n"
                'Content-Disposition: form-data; name="role"\r\n'
                "\r\n"
                "admin\r\n"
                f"--{boundary}--\r\n"
                ).encode("utf-8")

        headers = {
            "Accept": "*/*",
            "Content-Type": f"multipart/form-data; boundary={boundary}",
            "Content-Length": str(len(data)),  # keep if http_request doesn't auto-set it
        }

        response = self.web_server.http_request(
            uri="/test_multipart?type=simple_names_attributes",
            method="POST",
            headers=headers,
            data=data,  # body goes here
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b"name : Ivan") != -1)
        self.assertTrue(response.content.find(b"role : admin") != -1)

    def test_multipart_filename_attribute(self):
        boundary = "------------------------d74496d66958873e"

        file_bytes = b"Hello from test.txt\nSecond line\n"

        data = (f"--{boundary}\r\n"
                'Content-Disposition: form-data; name="file"; filename="test.txt"\r\n'
                "Content-Type: text/plain\r\n"
                "\r\n"
                ).encode("utf-8") + file_bytes + (
                   "\r\n"
                   f"--{boundary}--\r\n"
               ).encode("utf-8")

        headers = {
            "Accept": "*/*",
            "Content-Type": f"multipart/form-data; boundary={boundary}",
            "Content-Length": str(len(data)),
        }

        response = self.web_server.http_request(
            uri="/test_multipart?type=simple_file_attribute",
            method="POST",
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b"filename : test.txt") != -1)
        self.assertTrue(response.content.find(b"Hello from test.txt") != -1)

    def test_multipart_filename_array_attribute(self):
        boundary = "------------------------d74496d66958873e"

        # Two "files" (their raw bytes)
        file1_name = "a.txt"
        file1_bytes = b"Hello from a.txt\n"

        file2_name = "b.txt"
        file2_bytes = b"Hello from b.txt\n"

        # Array-style field name: files[]
        data = (f"--{boundary}\r\n"
                f'Content-Disposition: form-data; name="files[]"; filename="{file1_name}"\r\n'
                "Content-Type: text/plain\r\n"
                "\r\n"
                ).encode("utf-8") + file1_bytes + (
                   "\r\n"
                   f"--{boundary}\r\n"
                   f'Content-Disposition: form-data; name="files[]"; filename="{file2_name}"\r\n'
                   "Content-Type: text/plain\r\n"
                   "\r\n"
               ).encode("utf-8") + file2_bytes + (
                   "\r\n"
                   f"--{boundary}--\r\n"
               ).encode("utf-8")

        headers = {
            "Accept": "*/*",
            "Content-Type": f"multipart/form-data; boundary={boundary}",
            "Content-Length": str(len(data)),
        }

        response = self.web_server.http_request(
            uri="/test_multipart?type=file_array_attribute",
            method="POST",
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b"Hello from a.txt") != -1)
        self.assertTrue(response.content.find(b"Hello from b.txt") != -1)

    def test_multipart_name_urlencoded_attribute(self):
        boundary = "------------------------d74496d66958873e"

        # Part with explicit Content-Type: application/x-www-form-urlencoded
        # (this is still multipart/form-data overall; only this part is urlencoded)
        urlencoded_part = b"name=Ivan+Petrov&role=admin&note=a%26b%3Dc%25"

        data = (f"--{boundary}\r\n"
                   'Content-Disposition: form-data; name="form"\r\n'
                   "Content-Type: application/x-www-form-urlencoded; charset=UTF-8\r\n"
                   "\r\n"
               ).encode("utf-8") + urlencoded_part + (
                   "\r\n"
                   f"--{boundary}--\r\n"
               ).encode("utf-8")

        headers = {
            "Accept": "*/*",
            "Content-Type": f"multipart/form-data; boundary={boundary}",
            "Content-Length": str(len(data)),  # omit if your http_request sets it
        }

        response = self.web_server.http_request(
            uri="/test_multipart?type=name_urlencoded_attribute",
            method="POST",
            headers=headers,
            data=data,  # raw body bytes
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b"Ivan Petrov") != -1)
        self.assertTrue(response.content.find(b"a&b=c%") != -1)
