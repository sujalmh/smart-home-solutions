"""
Simulate Flutter frontend WebSocket + REST flow.
Tests ON/OFF commands for all 3 slaves and listens for response events.
Uses correct comp="Comp0" matching the slave firmware naming convention.
"""
import asyncio
import json
import time
import aiohttp

BASE = "http://127.0.0.1:8000"
WS_URL = "ws://127.0.0.1:8000/ws"
EMAIL = "test@local.dev"
PWD = "test1234"
SERVER = "RSW-0001"
SLAVES = ["9001", "9002", "9003"]
COMP = "Comp0"  # Slave firmware names components: Comp0, Comp1, Comp2, Comp3
LISTEN_SECS = 8  # seconds to wait for hardware response per step


async def main():
    async with aiohttp.ClientSession() as http:
        # ── LOGIN ──
        print("=" * 60)
        print("  SMART HOME E2E SIMULATION")
        print("=" * 60)
        print()

        resp = await http.post(f"{BASE}/api/auth/login", json={"email_id": EMAIL, "pwd": PWD})
        token = (await resp.json())["access_token"]
        headers = {"Authorization": f"Bearer {token}"}
        print(f"[AUTH] Logged in, token: {token[:30]}...")
        print()

        # ── OPEN WEBSOCKET (like Flutter SocketClient) ──
        ws = await http.ws_connect(f"{WS_URL}?token={token}")
        print("[WS] WebSocket connected to backend")
        print()

        received_events = []

        async def listen_ws(duration: float):
            """Collect WS events for `duration` seconds."""
            deadline = time.time() + duration
            while time.time() < deadline:
                remaining = deadline - time.time()
                if remaining <= 0:
                    break
                wait = min(remaining, 0.5)
                try:
                    msg = await asyncio.wait_for(ws.receive(), timeout=wait)
                    if msg.type == aiohttp.WSMsgType.TEXT:
                        parsed = json.loads(msg.data)
                        received_events.append(parsed)
                        evt = parsed.get("event", "?")
                        data = parsed.get("data", {})
                        dev = data.get("devID", "?")
                        comp = data.get("comp", "?")
                        mod = data.get("mod", "?")
                        stat = data.get("stat", "?")
                        val = data.get("val", "?")
                        ts = data.get("ts", "?")
                        print(f"  [WS <<] {evt}: dev={dev} comp={comp} mod={mod} stat={stat} val={val} ts={ts}")
                    elif msg.type in (aiohttp.WSMsgType.CLOSED, aiohttp.WSMsgType.ERROR):
                        print(f"  [WS] Connection closed/error: {msg}")
                        return
                except (asyncio.TimeoutError, asyncio.CancelledError):
                    continue

        # ── STEP 1: REQUEST INITIAL STATUS ──
        print("-" * 60)
        print(f"  STEP 1: REQUEST STATUS FROM ALL SLAVES (comp={COMP})")
        print("-" * 60)
        for dev_id in SLAVES:
            body = {"serverID": "0001", "devID": dev_id, "comp": COMP}
            resp = await http.post(f"{BASE}/api/devices/{SERVER}/status", json=body, headers=headers)
            print(f"  [REST ->] Status request {dev_id}: HTTP {resp.status}")

        print()
        print(f"  Listening for staresult events ({LISTEN_SECS}s)...")
        await listen_ws(LISTEN_SECS)
        print()

        # ── STEP 2: TURN ON ALL SLAVES ──
        print("-" * 60)
        print("  STEP 2: TURN ON ALL SLAVES (stat=1, val=1)")
        print("-" * 60)
        for dev_id in SLAVES:
            body = {"serverID": "0001", "devID": dev_id, "comp": COMP, "mod": 0, "stat": 1, "val": 1}
            resp = await http.post(f"{BASE}/api/devices/{SERVER}/command", json=body, headers=headers)
            result = await resp.json()
            print(f"  [REST ->] ON  {dev_id}: HTTP {resp.status} -> {json.dumps(result)}")

        print()
        print(f"  Listening for response events ({LISTEN_SECS}s)...")
        await listen_ws(LISTEN_SECS)
        print()

        # ── STEP 3: REQUEST STATUS AGAIN (should show ON) ──
        print("-" * 60)
        print("  STEP 3: VERIFY STATUS AFTER ON")
        print("-" * 60)
        for dev_id in SLAVES:
            body = {"serverID": "0001", "devID": dev_id, "comp": COMP}
            resp = await http.post(f"{BASE}/api/devices/{SERVER}/status", json=body, headers=headers)
            print(f"  [REST ->] Status request {dev_id}: HTTP {resp.status}")

        print()
        print(f"  Listening for staresult events ({LISTEN_SECS}s)...")
        await listen_ws(LISTEN_SECS)
        print()

        # ── STEP 4: TURN OFF ALL SLAVES ──
        print("-" * 60)
        print("  STEP 4: TURN OFF ALL SLAVES (stat=0, val=0)")
        print("-" * 60)
        for dev_id in SLAVES:
            body = {"serverID": "0001", "devID": dev_id, "comp": COMP, "mod": 0, "stat": 0, "val": 0}
            resp = await http.post(f"{BASE}/api/devices/{SERVER}/command", json=body, headers=headers)
            result = await resp.json()
            print(f"  [REST ->] OFF {dev_id}: HTTP {resp.status} -> {json.dumps(result)}")

        print()
        print(f"  Listening for response events ({LISTEN_SECS}s)...")
        await listen_ws(LISTEN_SECS)
        print()

        # ── STEP 5: FINAL STATUS CHECK ──
        print("-" * 60)
        print("  STEP 5: VERIFY STATUS AFTER OFF")
        print("-" * 60)
        for dev_id in SLAVES:
            body = {"serverID": "0001", "devID": dev_id, "comp": COMP}
            resp = await http.post(f"{BASE}/api/devices/{SERVER}/status", json=body, headers=headers)
            print(f"  [REST ->] Status request {dev_id}: HTTP {resp.status}")

        print()
        print(f"  Listening for staresult events ({LISTEN_SECS}s)...")
        await listen_ws(LISTEN_SECS)
        print()

        # ── CHECK DB MODULE STATE ──
        print("-" * 60)
        print("  STEP 6: READ MODULE STATE FROM DB (REST)")
        print("-" * 60)
        for dev_id in SLAVES:
            full_id = f"RSW-{dev_id}"
            resp = await http.get(f"{BASE}/api/clients/{full_id}/modules", headers=headers)
            modules = await resp.json()
            print(f"  [REST <-] {full_id} modules: {json.dumps(modules)}")
        print()

        # ── SUMMARY ──
        print("=" * 60)
        print(f"  SUMMARY: {len(received_events)} WebSocket events received")
        print("=" * 60)
        for i, evt in enumerate(received_events, 1):
            e = evt.get("event", "?")
            d = evt.get("data", {})
            print(f"  {i}. {e}: dev={d.get('devID','?')} comp={d.get('comp','?')} stat={d.get('stat','?')} val={d.get('val','?')}")
        print()

        await ws.close()
        print("[WS] Disconnected")
        print("DONE.")


if __name__ == "__main__":
    asyncio.run(main())
