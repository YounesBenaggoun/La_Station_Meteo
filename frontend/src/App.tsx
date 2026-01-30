import React, { useEffect, useRef, useState } from "react";

type WeatherVisualType = "sun" | "cloud" | "rain" | "snow";

interface WeatherVisual {
  label: string;
  description: string;
  skyClass: string;
  type: WeatherVisualType;
}

function formatDate(date: Date | null): string {
  if (!date) return "--";
  return date.toLocaleTimeString("fr-FR", {
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit"
  });
}

export const WeatherApp: React.FC = () => {
  const [temperature, setTemperature] = useState<number | null>(null);
  const [humidity, setHumidity] = useState<number | null>(null);
  const [unit, setUnit] = useState<"C" | "F">("C");
  const [simulation, setSimulation] = useState<boolean>(true);
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null);
  const [connected, setConnected] = useState<boolean>(false);
  const [error, setError] = useState<string | null>(null);
  const [sending, setSending] = useState<boolean>(false);

  const wsRef = useRef<WebSocket | null>(null);
  const reconnectTimeoutRef = useRef<number | null>(null);

  useEffect(() => {
    function connect() {
      const ws = new WebSocket("ws://localhost:8080");
      wsRef.current = ws;

      ws.onopen = () => {
        setConnected(true);
        setError(null);
      };

      ws.onclose = () => {
        setConnected(false);
        wsRef.current = null;
        if (reconnectTimeoutRef.current) {
          window.clearTimeout(reconnectTimeoutRef.current);
        }
        reconnectTimeoutRef.current = window.setTimeout(connect, 2000);
      };

      ws.onerror = () => {
        setError("Impossible de se connecter au bridge WebSocket.");
      };

      ws.onmessage = (event: MessageEvent) => {
        try {
          const data = JSON.parse(event.data);
          setTemperature(
            typeof data.temperature === "number" ? data.temperature : null
          );
          setHumidity(typeof data.humidity === "number" ? data.humidity : null);

          if (data.unit === "C" || data.unit === "F") {
            setUnit(data.unit);
          }
          if (typeof data.simulation === "boolean") {
            setSimulation(data.simulation);
          }
          setLastUpdate(new Date());
        } catch (e) {
          console.error("Payload invalide reçu :", e);
        }
      };
    }

    connect();

    return () => {
      if (reconnectTimeoutRef.current) {
        window.clearTimeout(reconnectTimeoutRef.current);
      }
      if (wsRef.current) {
        wsRef.current.close();
      }
    };
  }, []);

  const sendUnit = (newUnit: "C" | "F") => {
    if (!wsRef.current || wsRef.current.readyState !== WebSocket.OPEN) {
      setError("Bridge WebSocket non connecté.");
      return;
    }
    setSending(true);
    setError(null);

    wsRef.current.send(
      JSON.stringify({
        type: "setUnit",
        unit: newUnit
      })
    );

    setUnit(newUnit);
    window.setTimeout(() => setSending(false), 400);
  };

  const displayTemp =
    temperature != null ? `${temperature.toFixed(1)}` : "--";
  const displayHum =
    humidity != null ? `${humidity.toFixed(1)} %` : "--";

  let tempC = temperature;
  if (temperature != null && unit === "F") {
    tempC = ((temperature - 32) * 5) / 9;
  }

  const computeWeather = (temp: number | null, hum: number | null): WeatherVisual => {
    if (temp == null || hum == null) {
      return {
        label: "En attente des données…",
        description: "Connexion à la station météo",
        skyClass: "sky--cloudy",
        type: "cloud"
      };
    }

    if (temp <= 0) {
      return {
        label: "Neige",
        description: "Air très froid, risque de chutes de neige",
        skyClass: "sky--snowy",
        type: "snow"
      };
    }

    if (hum >= 85) {
      return {
        label: "Pluie forte",
        description: "Atmosphère très humide, précipitations probables",
        skyClass: "sky--rainy",
        type: "rain"
      };
    }

    if (hum >= 60) {
      return {
        label: "Pluie légère",
        description: "Nuages chargés, averses possibles",
        skyClass: "sky--rainy",
        type: "rain"
      };
    }

    if (temp >= 25 && hum < 60) {
      return {
        label: "Temps ensoleillé",
        description: "Air chaud et sec, ciel dégagé",
        skyClass: "sky--sunny",
        type: "sun"
      };
    }

    if (temp >= 15) {
      return {
        label: "Éclaircies",
        description: "Ciel partiellement nuageux",
        skyClass: "sky--cloudy",
        type: "cloud"
      };
    }

    return {
      label: "Nuageux",
      description: "Air frais, couverture nuageuse dominante",
      skyClass: "sky--cloudy",
      type: "cloud"
    };
  };

  const weather = computeWeather(tempC, humidity);

  const renderWeatherIcon = () => {
    if (weather.type === "snow") {
      return (
        <div className="weather-icon">
          <div className="cloud" />
          <div className="flakes">
            <span className="flake" />
            <span className="flake" />
            <span className="flake" />
          </div>
        </div>
      );
    }

    if (weather.type === "rain") {
      return (
        <div className="weather-icon">
          <div className="cloud" />
          <div className="raindrops">
            <span className="drop" />
            <span className="drop" />
            <span className="drop" />
          </div>
        </div>
      );
    }

    if (weather.type === "sun") {
      return (
        <div className="weather-icon">
          <div className="sun" />
        </div>
      );
    }

    return (
      <div className="weather-icon">
        <div className="cloud" />
      </div>
    );
  };

  return (
    <div className="app">
      <div className={`sky ${weather.skyClass}`} />

      {renderWeatherIcon()}

      <div className="card">
        <div className="header">
          <div>
            <div className="title">Station météo connectée</div>
            <div className="subtitle">
              {weather.label} · {weather.description}
            </div>
          </div>
          <div className="chip">
            <span className="chip-dot" />
            {connected ? "En ligne" : "Hors ligne"}
          </div>
        </div>

        <div className="main-row">
          <div className="temp-block">
            <div>
              <span className="temp-value">{displayTemp}</span>
              <span className="temp-unit">°{unit}</span>
            </div>
            <div style={{ fontSize: "0.8rem", color: "#9ca3af" }}>
              Température ressentie
            </div>
          </div>

          <div className="humidity-pill">
            <span className="humidity-dot" />
            <span>Humidité</span>
            <strong>{displayHum}</strong>
          </div>
        </div>

        <div className="meta-row">
          <div>
            <div className="meta-label">Dernière mise à jour</div>
            <div className="meta-value">{formatDate(lastUpdate)}</div>
          </div>
          <div className="badges">
            <div className={`badge ${simulation ? "badge-sim" : ""}`}>
              {simulation ? "Mode simulation" : "Capteur réel"}
            </div>
          </div>
        </div>

        <div className="controls">
          <div className="unit-toggle">
            <button
              className={`unit-btn ${unit === "C" ? "active" : ""}`}
              onClick={() => sendUnit("C")}
              disabled={sending}
            >
              °C
            </button>
            <button
              className={`unit-btn ${unit === "F" ? "active" : ""}`}
              onClick={() => sendUnit("F")}
              disabled={sending}
            >
              °F
            </button>
          </div>

          <div className="status-text">
            LEDs ESP32 : <span>{unit === "C" ? "Celsius" : "Fahrenheit"}</span>
          </div>
        </div>

        {error && <div className="error">{error}</div>}
      </div>
    </div>
  );
};

