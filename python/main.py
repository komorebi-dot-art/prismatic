from pathlib import Path
from typing import Optional

import mitmproxy.addonmanager
from adblock import FilterSet, Engine
from mitmproxy import http
from mitmproxy.script import concurrent
from mitmproxy.tools.main import mitmdump
from pythonosc.udp_client import SimpleUDPClient

engine: Optional[Engine] = None
udp_client: Optional[SimpleUDPClient] = None


def load_filter(filter_set: FilterSet, file) -> None:
    print(f"Loading Blocklist: {file.name}")
    with open(file, "r") as f:
        filter_set.add_filter_list(f.read())


def blink(it: bool = False) -> None:
    udp_client is not None and udp_client.send_message("/blocked", it)


def block(flow: http.HTTPFlow) -> None:
    flow.kill()
    blink(True)
    print(f"Blocked: {flow.request.url}")


def load(_: mitmproxy.addonmanager.Loader) -> None:
    global engine, udp_client
    fs = FilterSet()
    [load_filter(fs, it) for it in Path("./filters").rglob("*.txt")]
    engine = Engine(fs)
    udp_client = SimpleUDPClient("prismatic.local", 1337)


@concurrent
def requestheaders(flow: http.HTTPFlow) -> None:
    r = flow.request
    headers = r.headers
    source = headers.get("Origin") or headers.get("Referer")
    blink()
    print(f"Host: {r.url} | Source: {source or r.url} | Method: {r.method}")
    engine is not None and engine.check_network_urls(r.url, source or r.url, r.method).matched and block(flow)


if __name__ == "__main__":
    mitmdump(args=["-s", "main.py"])
