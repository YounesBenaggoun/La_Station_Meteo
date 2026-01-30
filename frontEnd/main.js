let uri = "ws://localhost:8080";
let status = "idle";
window.id = 0;

var mysock;


class Timer {
    t0 = 0;
    label = "";
    id = 0;
    constructor(label) {
        this.label = label;
        this.t0 = Date.now();
        console.log("Begin ", this.label);
    }
    end() {
        var diff = (Date.now() - this.t0) / 1000;
        console.log("end", this.label, diff);
    }

}

class mySocket {
    obj = false;
    compteur = 0;
    id = 0;
    timeoutDelay;
    constructor(uri, id) {
        this.uri = uri;
        this.id = id;
    }
    connect() {
        if (this.obj && this.obj.readyState <= 1) {
            return false;
        }
        this.obj = new WebSocket(this.uri);
        console.log("try To Connect N°", ++this.compteur);
        this.initEvent();
        this.timeOut();
    }
    autoConnect() {
        setTimeout(() => {
            this.connect();
        }, 5000);
    }
    send(message, to) {
        var hand = {
            "text": message
        }
        if (to) {
            hand.to = to;
        }
        this.obj.send(JSON.stringify(hand));
    }
    timeOut() {
        this.timeoutDelay = setTimeout(function () {
            if (this.obj.readyState != WebSocket.OPEN) {
                this.obj.close();
            }
        }.bind(this), 10000);
    }
    initEvent() {
        this.obj.onopen = () => {
            console.log("Connected...");
            clearTimeout(this.timeoutDelay);
            var hand = {
                "id": this.id,
                "first": true
            }
            this.obj.send(JSON.stringify(hand));
        };
        this.obj.onclose = (arg) => {
            console.log("CLOSE Event", this.obj.readyState);
            clearTimeout(this.timeoutDelay);
            this.autoConnect();
        };

        this.obj.onerror = (error) => {
            console.log("ERROR Event", this.obj.readyState);
        };
        this.obj.onmessage = (e) => {
            console.log(e.data);
            showResult(e.data);
        };
    }
}

function showResult(data) {
    // var data = '{"temp":"40.12","degreeMode":0,"humidity":"42.65"}';
    json = JSON.parse(data);
    console.log(json);

    let temp_html = document.querySelector("#temp");
    temp_html.innerHTML = json.temp;

    let unit_html = document.querySelector("#unit");
    if (json.degreeMode) {
        unit_html.innerHTML = "°";
    } else {
        unit_html.innerHTML = "F";
    }

    let humidity = document.querySelector("#humidity");
    humidity.innerHTML = json.humidity;
}

window.onload = function () {
    if (!mysock) {
        mysock = new mySocket(uri, 1);
        mysock.connect();
    }
}