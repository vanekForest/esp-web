import logging
from flask import Flask, jsonify
import requests
import threading
import time

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
log = logging.getLogger()

app = Flask(__name__)

ESP32_URL = "http://192.168.31.23/api/temp"
FETCH_INTERVAL = 2

latest_temp = None
temp_sum = 0.0
temp_count = 0


def fetch_from_esp32():
    global latest_temp, temp_sum, temp_count

    while True:
        try:
            response = requests.get(ESP32_URL, timeout=5)
            if response.ok:
                data = response.json()
                temp = float(data.get("current", -127))

                if temp == -127:
                    log.warning("Received invalid temperature (-127) from ESP32")
                else:
                    latest_temp = temp
                    temp_sum += temp
                    temp_count += 1
                    log.info(f"Valid temperature received: {temp:.1f}°C")
            else:
                log.warning(f"Bad HTTP response from ESP32: {response.status_code}")
        except Exception as e:
            log.error(f"Exception while fetching temperature: {e}")

        time.sleep(FETCH_INTERVAL)


@app.route("/api/temp", methods=["GET"])
def get_temp():
    if temp_count == 0 or latest_temp is None:
        result = {"current": None, "average": 0.0}
    else:
        average = round(temp_sum / temp_count, 1)
        result = {"current": round(latest_temp, 1), "average": average}

    log.info(f"Responding with: {result}")
    return jsonify(result)


@app.route("/api/reset", methods=["POST"])
def reset_avg():
    global temp_sum, temp_count
    temp_sum = 0.0
    temp_count = 0
    log.info("Average temperature reset")
    return '', 204


if __name__ == "__main__":
    threading.Thread(target=fetch_from_esp32, daemon=True).start()
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False, threaded=True)
