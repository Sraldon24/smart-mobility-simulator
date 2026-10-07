import React from "react";
import { AlertTriangle, Plus, X } from "lucide-react";

export default function IncidentPanel({
  incidents,
  incidentFrom,
  setIncidentFrom,
  incidentTo,
  setIncidentTo,
  incidentType,
  setIncidentType,
  onAddIncident,
  onRemoveIncident,
  incidentError,
}) {
  return (
    <div className="flex-col gap-md">
      <h2>Traffic Incidents</h2>

      <div className="flex-row gap-sm">
        <input
          type="number"
          value={incidentFrom}
          onChange={(e) => setIncidentFrom(e.target.value)}
          placeholder="From"
          style={{ width: "80px" }}
        />
        <span className="text-muted">→</span>
        <input
          type="number"
          value={incidentTo}
          onChange={(e) => setIncidentTo(e.target.value)}
          placeholder="To"
          style={{ width: "80px" }}
        />
      </div>

      <div className="flex-row gap-sm" style={{ marginTop: "0.5rem" }}>
        <select
          value={incidentType}
          onChange={(e) => setIncidentType(e.target.value)}
          style={{ flex: 1 }}
        >
          <option value="closure">Road Closure</option>
          <option value="accident">Accident</option>
        </select>
        <button
          className="btn btn-primary"
          onClick={onAddIncident}
          disabled={
            incidentFrom === "" ||
            incidentTo === "" ||
            incidentFrom === incidentTo
          }
        >
          <Plus size={16} /> Add
        </button>
      </div>

      {incidentError && (
        <div className="text-sm" style={{ color: "var(--danger)" }}>
          {incidentError}
        </div>
      )}

      {incidents.length > 0 && (
        <div className="flex-col gap-sm" style={{ marginTop: "1rem" }}>
          <h3 className="text-sm text-secondary">Active Incidents</h3>
          {incidents.map((inc, i) => (
            <div
              key={i}
              className="card flex-row"
              style={{ justifyContent: "space-between", padding: "0.5rem" }}
            >
              <div className="flex-row gap-sm">
                <AlertTriangle
                  size={14}
                  className={
                    inc.type === "closure" ? "text-danger" : "text-warning"
                  }
                />
                <span className="text-sm">
                  {inc.from} → {inc.to}
                  <span
                    className="text-muted"
                    style={{
                      marginLeft: "6px",
                      fontSize: "0.75rem",
                      textTransform: "uppercase",
                    }}
                  >
                    ({inc.type})
                  </span>
                </span>
              </div>
              <button
                className="btn"
                style={{
                  padding: "4px",
                  background: "transparent",
                  border: "none",
                }}
                onClick={() => onRemoveIncident(inc.from, inc.to)}
              >
                <X size={14} className="text-muted" />
              </button>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
