import os
import re
from urllib.parse import urlencode

from python.lib.testcase import WebServerAutoTestCase

def get_multipart_temp_files():
    """Get files that look like multipart temp files (6 random alphanumeric chars in /tmp/)."""
    pattern = re.compile(r'^[a-zA-Z0-9]{6}$')
    return set(f for f in os.listdir("/tmp/") if pattern.match(f) and os.path.isfile(f"/tmp/{f}"))


class TestMultipartContentType(WebServerAutoTestCase):

    def test_multipart_quoted_boundary(self):
        """Test that quoted boundary in Content-Type header is handled correctly."""
        boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW"

        data = (f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="name"\r\n'
                '\r\n'
                'John\r\n'
                f'--{boundary}--\r\n'
                ).encode('utf-8')

        # Boundary is quoted in Content-Type header
        headers = {
            'Accept': '*/*',
            'Content-Type': f'multipart/form-data; boundary="{boundary}"',
            'Content-Length': str(len(data)),
        }

        response = self.web_server.http_request(
            uri='/test_multipart?type=quoted_boundary',
            method='POST',
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b'name : John') != -1)

    def test_multipart_boundary_with_charset(self):
        """Test that boundary with additional params (charset) is parsed correctly."""
        boundary = '------------------------d74496d66958873e'

        data = (f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="data"\r\n'
                '\r\n'
                'test value\r\n'
                f'--{boundary}--\r\n'
                ).encode('utf-8')

        # Boundary with charset after it
        headers = {
            'Accept': '*/*',
            'Content-Type': f'multipart/form-data; boundary={boundary}; charset=UTF-8',
            'Content-Length': str(len(data)),
        }

        response = self.web_server.http_request(
            uri='/test_multipart?type=boundary_with_charset',
            method='POST',
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b'data : test value') != -1)

    def test_multipart_empty_value(self):
        """Test that empty form field values are handled correctly."""
        boundary = '------------------------d74496d66958873e'

        data = (f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="empty_field"\r\n'
                '\r\n'
                '\r\n'  # Empty value
                f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="non_empty"\r\n'
                '\r\n'
                'value\r\n'
                f'--{boundary}--\r\n'
                ).encode('utf-8')

        headers = {
            'Accept': '*/*',
            'Content-Type': f'multipart/form-data; boundary={boundary}',
            'Content-Length': str(len(data)),
        }

        response = self.web_server.http_request(
            uri='/test_multipart?type=empty_value',
            method='POST',
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b'empty_field :') != -1)
        self.assertTrue(response.content.find(b'non_empty : value') != -1)

    def test_multipart_special_chars_in_name(self):
        """Test form field names with special characters."""
        boundary = '------------------------d74496d66958873e'

        data = (f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="name_with_underscore"\r\n'
                '\r\n'
                'value1\r\n'
                f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="name-with-dash"\r\n'
                '\r\n'
                'value2\r\n'
                f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="name.with.dots"\r\n'
                '\r\n'
                'value3\r\n'
                f'--{boundary}--\r\n'
                ).encode('utf-8')

        headers = {
            'Accept': '*/*',
            'Content-Type': f'multipart/form-data; boundary={boundary}',
            'Content-Length': str(len(data)),
        }

        response = self.web_server.http_request(
            uri='/test_multipart?type=special_chars_in_name',
            method='POST',
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b'value1') != -1)
        self.assertTrue(response.content.find(b'value2') != -1)
        self.assertTrue(response.content.find(b'value3') != -1)

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

    def test_multipart_non_terminating_boundary(self):
        boundary = "------------------------d74496d66958873e"

        data = (f"--{boundary}\r\n"
                'Content-Disposition: form-data; name="name"\r\n'
                "\r\n"
                "Ivan\r\n"
                f"--{boundary}\r\n"
                ).encode("utf-8")

        headers = {
            "Accept": "*/*",
            "Content-Type": f"multipart/form-data; boundary={boundary}; charset=UTF-8",
            "Content-Length": str(len(data)),  # keep if http_request doesn't auto-set it
        }

        response = self.web_server.http_request(
            uri="/test_multipart?type=non_terminating_boundary",
            method="POST",
            headers=headers,
            data=data,  # body goes here
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b"name : Ivan") != -1)

    def test_multipart_filename_attribute(self):

        tmp_files = get_multipart_temp_files()
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

        tmp_files_after_script = get_multipart_temp_files()
        # check that script delete tmp files at the end
        self.assertEqual(sorted(tmp_files), sorted(tmp_files_after_script))

    def test_multipart_filename_array_attribute(self):
        tmp_files = get_multipart_temp_files()

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

        tmp_files_after_script = get_multipart_temp_files()
        # check that script delete tmp files at the end
        self.assertEqual(sorted(tmp_files), sorted(tmp_files_after_script))

    def test_multipart_superglobal_modify(self):

        tmp_files = get_multipart_temp_files()
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
            uri="/test_multipart?type=superglobal_modify",
            method="POST",
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)

        tmp_files_after_script = get_multipart_temp_files()
        # check that script delete tmp files at the end
        self.assertEqual(sorted(tmp_files), sorted(tmp_files_after_script))

    def test_multipart_mixed_files_and_fields(self):
        """Test mixing files and regular form fields in the same request."""
        tmp_files = get_multipart_temp_files()
        boundary = '------------------------d74496d66958873e'

        file_bytes = b'File content here\n'

        data = (f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="text_field"\r\n'
                '\r\n'
                'text value\r\n'
                f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="upload"; filename="test.txt"\r\n'
                'Content-Type: text/plain\r\n'
                '\r\n'
                ).encode('utf-8') + file_bytes + (
                   f'\r\n--{boundary}\r\n'
                   'Content-Disposition: form-data; name="another_field"\r\n'
                   '\r\n'
                   'another value\r\n'
                   f'--{boundary}--\r\n'
               ).encode('utf-8')

        headers = {
            'Accept': '*/*',
            'Content-Type': f'multipart/form-data; boundary={boundary}',
            'Content-Length': str(len(data)),
        }

        response = self.web_server.http_request(
            uri='/test_multipart?type=mixed_files_and_fields',
            method='POST',
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b'text : text value') != -1)
        self.assertTrue(response.content.find(b'filename : test.txt') != -1)
        self.assertTrue(response.content.find(b'another : another value') != -1)

        tmp_files_after_script = get_multipart_temp_files()
        # check that script delete tmp files at the end
        self.assertEqual(sorted(tmp_files), sorted(tmp_files_after_script))

    def test_multipart_file_without_content_type(self):
        """Test file upload without explicit Content-Type header."""
        tmp_files = get_multipart_temp_files()
        boundary = '------------------------d74496d66958873e'

        file_bytes = b'File without content type\n'

        # No Content-Type header for the file part
        data = (f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="file"; filename="no_type.txt"\r\n'
                '\r\n'
                ).encode('utf-8') + file_bytes + (
                   f'\r\n--{boundary}--\r\n'
               ).encode('utf-8')

        headers = {
            'Accept': '*/*',
            'Content-Type': f'multipart/form-data; boundary={boundary}',
            'Content-Length': str(len(data)),
        }

        response = self.web_server.http_request(
            uri='/test_multipart?type=file_without_content_type',
            method='POST',
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b'filename : no_type.txt') != -1)
        # Should default to text/plain
        self.assertTrue(response.content.find(b'type : text/plain') != -1)

        tmp_files_after_script = get_multipart_temp_files()
        # check that script delete tmp files at the end
        self.assertEqual(sorted(tmp_files), sorted(tmp_files_after_script))

    def test_multipart_binary_file(self):
        """Test uploading binary file content."""
        tmp_files = get_multipart_temp_files()
        boundary = '------------------------d74496d66958873e'

        # Binary content with null bytes
        file_bytes = b'\x00\x01\x02\x03\xff\xfe\xfd\xfc'

        data = (f'--{boundary}\r\n'
                'Content-Disposition: form-data; name="binary"; filename="data.bin"\r\n'
                'Content-Type: application/octet-stream\r\n'
                '\r\n'
                ).encode('utf-8') + file_bytes + (
                   f'\r\n--{boundary}--\r\n'
               ).encode('utf-8')

        headers = {
            'Accept': '*/*',
            'Content-Type': f'multipart/form-data; boundary={boundary}',
            'Content-Length': str(len(data)),
        }

        response = self.web_server.http_request(
            uri='/test_multipart?type=binary_file',
            method='POST',
            headers=headers,
            data=data,
        )

        self.assertEqual(200, response.status_code)
        self.assertTrue(response.content.find(b'size : 8') != -1)
        self.assertTrue(response.content.find(b'filename : data.bin') != -1)

        tmp_files_after_script = get_multipart_temp_files()
        # check that script delete tmp files at the end
        self.assertEqual(sorted(tmp_files), sorted(tmp_files_after_script))

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
