import http.client
import base64
import json
import datetime
import sys
import creds.tidepool_creds
from zoneinfo import ZoneInfo

API_URL = "api.tidepool.org"


class Tidepool:
    def __init__(self):
        self.uid = None
        self.auth = None

    def connect(self, uname, passwd):
        self.auth = base64.b64encode(f"{uname}:{passwd}".encode("utf-8"))

        uid, token = self._auth()
        if not uid or not token:
            return None

        users = self._get_users(token, uid)
        if len(users) < 1:
            return None

        self.uid = users[0]
        return self

    def bg(self):
        _, token = self._auth()
        if token is None:
            return (None, None)

        return self._get_bg(token, self.uid)

    def _auth(self):
        if not self.auth:
            return (None, None)

        conn = http.client.HTTPSConnection(API_URL)

        payload = ""
        headers = {
            "Authorization": f"Basic {self.auth.decode('utf-8')}",
            "Content-Type": "application/json",
        }
        conn.request("POST", "/auth/login", payload, headers)
        res = conn.getresponse()

        if res.status == 200:
            data = res.read()
            uid = json.loads(data)["userid"]
            token = res.getheader("X-Tidepool-Session-Token")
            return (uid, token)

        print(f"Auth fail: {res.status}")
        return (None, None)

    def _get_users(self, token, uid):
        conn = http.client.HTTPSConnection(API_URL)

        payload = ""
        headers = {
            "X-Tidepool-Session-Token": f"{token}",
            "Content-Type": "application/json",
        }
        conn.request("GET", f"/metadata/users/{uid}/users", payload, headers)
        res = conn.getresponse()

        if res.status != 200:
            print(f"User fail: {res.status}")
            return []

        data = res.read()
        return list(map(lambda x: x["userid"], json.loads(data)))

    def _get_bg(self, token, user):
        conn = http.client.HTTPSConnection(API_URL)

        payload = ""
        headers = {
            "X-Tidepool-Session-Token": f"{token}",
            "Content-Type": "application/json",
        }
        conn.request("GET", f"/data/{user}?latest=true&type=cbg", payload, headers)
        res = conn.getresponse()

        if res.status != 200:
            print("bg fail")
            return (None, None)

        res_obj = json.loads(res.read())[0]
        bg = res_obj["value"]
        time = res_obj["time"]
        time = datetime.datetime.strptime(time, "%Y-%m-%dT%H:%M:%SZ").replace(
            tzinfo=ZoneInfo("UTC")
        )

        return (bg * 18.01559, time)


if __name__ == "__main__":
    tp = Tidepool().connect(creds.tidepool_creds.uname, creds.tidepool_creds.passwd)
    if not tp:
        sys.exit("Couldn't connect to tidepool")

    val, time = tp.bg()
    if val and time:
        time = time.astimezone(ZoneInfo("America/New_York"))
        time = time.strftime("%a %d %b %Y, %I:%M%p")
        print(f"{time} - {int(val)}")
    else:
        print("Couldn't get current value")
