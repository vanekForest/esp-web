import logging
from flask import Flask, jsonify, request
import requests
import threading
import time

logging.basicConfig(level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
log = logging.getLogger()

app = Flask(__name__)

ESP32_URL = "http://host.docker.internal:8083/api/temp"
FETCH_INTERVAL = 2

latest_indoor = None
latest_outdoor = None
indoor_sum = 0.0
outdoor_sum = 0.0
temp_count = 0


def fetch_from_esp32():
    global latest_indoor, latest_outdoor, indoor_sum, outdoor_sum, temp_count

    while True:
        try:
            response = requests.get(ESP32_URL, timeout=5)
            if response.ok:
                data = response.json()
                indoor = float(data.get("indoor", -127))
                outdoor = float(data.get("outdoor", -127))

                if indoor != -127 and outdoor != -127:
                    latest_indoor = indoor
                    latest_outdoor = outdoor
                    indoor_sum += indoor
                    outdoor_sum += outdoor
                    temp_count += 1
                    log.info(f"Indoor: {indoor:.1f}°C | Outdoor: {outdoor:.1f}°C")
                else:
                    log.warning("Invalid temperature received from ESP32")
            else:
                log.warning(f"Bad HTTP response: {response.status_code}")
        except Exception as e:
            log.error(f"Exception while fetching temperature: {e}")

        time.sleep(FETCH_INTERVAL)


@app.route("/api/temp", methods=["GET"])
def get_temp():
    if temp_count == 0 or latest_indoor is None or latest_outdoor is None:
        result = {"indoor": None, "outdoor": None, "indoor_avg": 0.0, "outdoor_avg": 0.0}
    else:
        result = {
            "indoor": round(latest_indoor, 1),
            "outdoor": round(latest_outdoor, 1),
            "indoor_avg": round(indoor_sum / temp_count, 1),
            "outdoor_avg": round(outdoor_sum / temp_count, 1)
        }

    log.info(f"Responding with: {result}")
    return jsonify(result)


@app.route("/api/reset", methods=["POST"])
def reset_avg():
    global indoor_sum, outdoor_sum, temp_count
    indoor_sum = 0.0
    outdoor_sum = 0.0
    temp_count = 0
    log.info("Averages reset")
    return '', 204


if __name__ == "__main__":
    threading.Thread(target=fetch_from_esp32, daemon=True).start()
    app.run(host="0.0.0.0", port=5000, debug=False, use_reloader=False, threaded=True)