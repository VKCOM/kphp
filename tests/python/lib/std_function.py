import json
import re
import typing


class Invocations:
    BUILTIN_CALLED_MSG_RE = re.compile(b"built-in called: ([\\w$]+)")

    def __init__(self, kphp_tracked_builtins_list: str):
        self._data = {builtin: 0 for builtin in kphp_tracked_builtins_list.split()}

    def update(self, logs: bytes):
        for match in self.BUILTIN_CALLED_MSG_RE.finditer(logs):
            self._data[match[1].decode()] += 1

    def dump(self, target_file: typing.TextIO):
        """
        Write the current statistics as JSON into the provided file descriptor.
        
        The caller is responsible for opening and closing the file.
        """
        json.dump(self._data, target_file, indent=4)
