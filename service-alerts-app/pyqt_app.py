import sys
import os
def resource_path(filename):
    if hasattr(sys, "_MEIPASS"):
        return os.path.join(sys._MEIPASS, filename)
    return os.path.join(os.path.abspath("."), filename)
from PyQt6.QtCore import QUrl, pyqtSignal, QTimer
import paho.mqtt.client as mqtt
from PyQt6.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QLabel
from PyQt6.QtGui import QFont, QPixmap, QPainter, QColor
from PyQt6.QtCore import Qt
import json, time

def interpolate_color(color1, color2, factor):
    r = color1.red() + (color2.red() - color1.red()) * factor
    g = color1.green() + (color2.green() - color1.green()) * factor
    b = color1.blue() + (color2.blue() - color1.blue()) * factor
    return QColor(int(r), int(g), int(b))

class GradientLabel(QLabel):
    def set_solid_color(self, color):
        """Set the label to a solid color and stop animation."""
        self.timer.stop()
        self.setStyleSheet(f"color: {QColor(color).name()}; background: transparent;")

    def start_gradient(self, color1, color2):
        """Start gradient animation between two colors."""
        self.color1 = QColor(color1)
        self.color2 = QColor(color2)
        self.factor = 0.0
        self.direction = 1
        self.timer.start(5)
    def __init__(self, text, font, color1, color2, parent=None):
        super().__init__(text, parent)
        self.setFont(font)
        self.color1 = QColor(color1)
        self.color2 = QColor(color2)
        self.factor = 0.0
        self.direction = 1
        self.setStyleSheet("background: transparent;")
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_color)
        self.timer.start(5)

    def update_color(self):
        self.factor += 0.01 * self.direction
        if self.factor >= 1.0:
            self.factor = 1.0
            self.direction = -1
        elif self.factor <= 0.0:
            self.factor = 0.0
            self.direction = 1
        color = interpolate_color(self.color1, self.color2, self.factor)
        self.setStyleSheet(f"color: {color.name()}; background: transparent;")

class MyApp(QWidget):
    button_event_signal = pyqtSignal(dict)
    kitchen_event_signal = pyqtSignal(list, int)
    mqtt_connect_failed_signal = pyqtSignal()
    watchok_signal = pyqtSignal()

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Cleaning Alert App")
        self.setGeometry(200, 200, 600, 400)

        self.last_button_array = None

        # Background image
        self.bg_pixmap = QPixmap(resource_path("img.png"))

        main_layout = QVBoxLayout()
        main_layout.setContentsMargins(20, 20, 20, 20)
        main_layout.addStretch(1)

        heading_font = QFont("Comic Sans MS", 28, QFont.Weight.Bold)
        self.heading_label = GradientLabel("Cleaning Alert App", heading_font, "#000000", "#B0B0B0")
        main_layout.addWidget(self.heading_label, alignment=Qt.AlignmentFlag.AlignHCenter)
        main_layout.addStretch(2)

        # MQTT setup
        from PyQt6.QtMultimedia import QSoundEffect
        self.mqtt_client = mqtt.Client()
        self.mqtt_client.on_connect = self.on_mqtt_connect
        self.mqtt_client.on_message = self.on_mqtt_message
        mqtt_host = "iotcore.cowlar.com"
        mqtt_port = 1883
        mqtt_user = "dockersim-dispenser"
        mqtt_pass = "CowlarGeyser7890"
        self.mqtt_client.username_pw_set(mqtt_user, mqtt_pass)
        self.mqtt_client.connect_async(mqtt_host, mqtt_port, 60)
        self.mqtt_client.loop_start()

        # Logic variables
        self.last_button_ts = None
        self.kitchen_timer = None
        self.kitchen_received = False
        self.sound_timer = None
        self.watchok_timer = None

        # Sounds
        self.sound = QSoundEffect()
        self.sound.setSource(QUrl.fromLocalFile(resource_path("alert.wav")))
        self.sound.setLoopCount(-2)
        self.sound.setVolume(1)
        self.escalate_sound = QSoundEffect()
        self.escalate_sound.setSource(QUrl.fromLocalFile(resource_path("escalate.wav")))
        self.escalate_sound.setLoopCount(1)
        self.escalate_sound.setVolume(1)

        # Status label
        status_font = QFont("Comic Sans MS", 22, QFont.Weight.Bold)
        self.status_label = GradientLabel("Waiting for Alert", status_font, "#000000", "#B0B0B0")
        main_layout.addWidget(self.status_label, alignment=Qt.AlignmentFlag.AlignHCenter)

        # Button
        button = QPushButton("   OK   ", self)
        button.setFont(QFont("Comic Sans MS", 18, QFont.Weight.Bold))
        button.setStyleSheet("""
            QPushButton {
                color: #FF0000;
                border: 2px solid #FFB6C1;
                background: rgba(255,255,255,150);
                border-radius: 10px;
                padding: 8px 16px;
            }
            QPushButton:hover {
                border: 2px solid #87CEEB;
                color: #013220;
            }
        """)
        button.clicked.connect(self.alert_triggered)
        main_layout.addWidget(button, alignment=Qt.AlignmentFlag.AlignHCenter)
        main_layout.addStretch(3)
        self.setLayout(main_layout)

        # Connect signals
        self.button_event_signal.connect(self.handle_button_event)
        self.kitchen_event_signal.connect(self.handle_kitchen_event)
        self.mqtt_connect_failed_signal.connect(self.handle_mqtt_connect_failed)
        self.watchok_signal.connect(self.handle_watchok)

    # --- Button click & WatchOK logic ---
    def alert_triggered(self):
        self.status_label.setText("Button Pressed!")
        self.sound.stop()  # stop current alert

        # Publish 1 to alert/watchok
        self.mqtt_client.publish("alert/watchok", "1")
        print("[MQTT] Published 1 to alert/watchok")

        # Start 10s timer waiting for watchok
        if self.watchok_timer:
            self.watchok_timer.stop()
        self.watchok_timer = QTimer(self)
        self.watchok_timer.setSingleShot(True)
        self.watchok_timer.timeout.connect(self.watchok_timeout)
        self.watchok_timer.start(10 * 1000)

    def handle_watchok(self):
        if self.watchok_timer:
            self.watchok_timer.stop()
        self.kitchen_received = True
        self.status_label.setText("Watch OK received")
        QTimer.singleShot(1000, lambda: None)
        self.status_label.setText("Waiting for Alert")
        print("[MQTT] Watch OK received")
        # Set all labels to solid black
        self.heading_label.set_solid_color("#000000")
        self.status_label.set_solid_color("#000000")

    def watchok_timeout(self):
        print("[Timer] No Watch OK received, escalating...")
        self.status_label.setText("Escalating!")
        self.escalate_sound.play()

        # After 3s, publish alert/button index
        def publish_ads():
            button_index = self.last_button_array if self.last_button_array else [1,0,0,0,0,0]
            self.mqtt_client.publish("alert/ads", json.dumps(button_index))
            print(f"[MQTT] Published {button_index} to alert/ads")

        QTimer.singleShot(3000, publish_ads)

    # --- MQTT handlers ---
    def on_mqtt_connect(self, client, userdata, flags, rc):
        if rc == 0:
            client.subscribe("alert/button")
            client.subscribe("alert/kitchen")
            client.subscribe("alert/watchok")
        else:
            self.mqtt_connect_failed_signal.emit()

    def on_mqtt_message(self, client, userdata, msg):
        topic = msg.topic
        payload = msg.payload.decode("utf-8")
        now = int(time.time())
        print(f"[MQTT] Topic: {topic} | Payload: {payload} | Now: {now}")

        if topic == "alert/button":
            try:
                data = json.loads(payload)
                ts = int(data.get("ts", 0))
                self.button_event_signal.emit({"data": data, "ts": ts, "now": now, "payload": payload})
            except Exception as e:
                print("[Button] Error parsing payload:", e)

        elif topic == "alert/kitchen":
            try:
                arr_str, ts_str = payload.rsplit(",", 1)
                arr = json.loads(arr_str)
                ts = int(ts_str)
                self.kitchen_event_signal.emit(arr, ts)
            except Exception as e:
                print("[Kitchen] Error parsing payload:", e)

        elif topic == "alert/watchok":
            self.watchok_signal.emit()

    # --- Slots ---
    def handle_button_event(self, event):
        ts = event["ts"]
        now = event["now"]
        payload = event["payload"]
        button_array = event["data"].get("button", None)
        if button_array:
            self.last_button_array = button_array
        if now - ts <= 80: #Bathroom button ts setup
            self.kitchen_received = False
            self.last_button_ts = ts
            if self.kitchen_timer:
                self.kitchen_timer.stop()
            self.kitchen_timer = QTimer(self)
            self.kitchen_timer.setSingleShot(True)
            self.kitchen_timer.timeout.connect(self.kitchen_timeout)
            self.kitchen_timer.start(2 * 60 * 1000)                           #after 2 min of no response from watch to topic alert/kitchen
            print(f"[Timer] Kitchen timer started at {time.strftime('%H:%M:%S')}, waiting for watch response...")
            self.status_label.setText("Button pressed, waiting for watch response")
            # Start gradient animation on all labels
            self.heading_label.start_gradient("#000000", "#B0B0B0")
            self.status_label.start_gradient("#000000", "#B0B0B0")
        else:
            self.status_label.setText("Button event too old.")
            QTimer.singleShot(1000, lambda: None)
            self.status_label.setText("Waiting for Alert")

    def handle_kitchen_event(self, arr, ts):
        if self.last_button_ts and not self.kitchen_received:
            if ts >= self.last_button_ts and (ts - self.last_button_ts) <= 5 * 60:   
                self.kitchen_received = True
                self.status_label.setText("Kitchen event received in time.")
                if self.kitchen_timer:
                    self.kitchen_timer.stop()
                if self.sound_timer:
                    self.sound_timer.stop()
                self.sound.stop()

    def handle_mqtt_connect_failed(self):
        self.status_label.setText("MQTT Connect Failed")


    def kitchen_timeout(self):
        print("[Timer] Kitchen timer expired. Starting alert.wav")
        washroom_text = "Waiting for alert!"
        if self.last_button_array:
            try:
                idx = self.last_button_array.index(1)
                washroom_text = f"Clean Washroom {idx+1}"
            except ValueError:
                pass
        self.status_label.setText(washroom_text)
        self.sound.play()

        # Stop previous sound timer if exists
        if self.sound_timer:
            self.sound_timer.stop()

        # Timer for 60 seconds waiting for watchok
        self.sound_timer = QTimer(self)
        self.sound_timer.setSingleShot(True)

        def wait_watchok_timeout():
            if not self.kitchen_received:  # No watchok received
                print("[Timer] 60s passed, no WatchOK. Starting escalate.wav")
                self.status_label.setText("Escalating!")
                self.sound.stop()  # stop alert.wav
                self.escalate_sound.play()

                # After 3 seconds, publish correct washroom index to alert/ads
                def publish_ads():
                    idx = 0
                    if self.last_button_array:
                        try:
                            idx = self.last_button_array.index(1)
                        except ValueError:
                            idx = 0
                    self.mqtt_client.publish("alert/ads", json.dumps(idx))
                    print(f"[MQTT] Published {idx} to alert/ads")
                    self.escalate_sound.stop()
                    self.status_label.setText("Escalation sent.")
                    QTimer.singleShot(1000, lambda: None)
                    self.status_label.setText("Waiting for Alert")

                QTimer.singleShot(3000, publish_ads)

        self.sound_timer.timeout.connect(wait_watchok_timeout)
        self.sound_timer.start(60 * 1000)    #alert.wav sound for 60s


    def stop_alert_sound(self):
        self.sound.stop()

    def paintEvent(self, event):
        if not self.bg_pixmap.isNull():
            painter = QPainter(self)
            scaled = self.bg_pixmap.scaled(
                self.size(),
                Qt.AspectRatioMode.KeepAspectRatioByExpanding,
                Qt.TransformationMode.SmoothTransformation
            )
            painter.drawPixmap(self.rect(), scaled)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = MyApp()
    window.show()
    sys.exit(app.exec())
