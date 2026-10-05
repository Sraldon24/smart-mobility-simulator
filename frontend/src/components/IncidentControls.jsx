import React from 'react';

export default function IncidentControls({ 
  incidentFrom, setIncidentFrom, 
  incidentTo, setIncidentTo, 
  incidentType, setIncidentType,
  incidentError, setIncidentError,
  handleAddIncident,
  incidents, handleClearIncident,
  roads
}) {
  const onAddClick = () => {
    if (incidentFrom === '' || incidentTo === '') {
      setIncidentError("Select both nodes for the incident.");
      return;
    }
    const from = parseInt(incidentFrom);
    const to = parseInt(incidentTo);
    if (from === to) {
      setIncidentError("Start and end node cannot be the same.");
      return;
    }
    const roadExists = roads.some(r => 
      (r.from === from && r.to === to) || (r.from === to && r.to === from)
    );
    if (!roadExists) {
      setIncidentError("Road does not exist between these nodes.");
      return;
    }
    handleAddIncident();
  };

  return (
    <div style={{ padding: '15px', backgroundColor: '#fdf3f2', borderRadius: '8px', border: '1px solid #f5b7b1' }}>
      <h2 style={{ marginTop: 0, fontSize: '1.2rem', color: '#c0392b' }}>Incidents</h2>
      
      <div style={{ display: 'flex', gap: '10px', marginBottom: '10px' }}>
        <div style={{ flex: 1 }}>
          <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>From:</label>
          <input type="number" value={incidentFrom} onChange={(e) => setIncidentFrom(e.target.value)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
        </div>
        <div style={{ flex: 1 }}>
          <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>To:</label>
          <input type="number" value={incidentTo} onChange={(e) => setIncidentTo(e.target.value)} style={{ width: '100%', boxSizing: 'border-box', padding: '5px' }} />
        </div>
      </div>

      <div style={{ marginBottom: '15px' }}>
        <label style={{ display: 'block', fontSize: '0.9rem', fontWeight: 'bold' }}>Type:</label>
        <select value={incidentType} onChange={(e) => setIncidentType(e.target.value)} style={{ width: '100%', padding: '5px' }}>
          <option value="closure">Closure</option>
          <option value="accident">Accident</option>
        </select>
      </div>

      {incidentError && (
        <p style={{ color: '#c0392b', margin: '5px 0', fontSize: '0.9rem' }}>{incidentError}</p>
      )}

      <button onClick={onAddClick} style={{ width: '100%', padding: '8px', backgroundColor: '#e74c3c', color: 'white', border: 'none', borderRadius: '4px', cursor: 'pointer', fontWeight: 'bold' }}>
        Add Incident
      </button>

      {incidents.length > 0 && (
        <div style={{ marginTop: '15px' }}>
          <h4 style={{ margin: '0 0 10px 0' }}>Active Incidents</h4>
          <ul style={{ paddingLeft: '20px', margin: 0, fontSize: '0.9rem' }}>
            {incidents.map((inc, i) => (
              <li key={i} style={{ marginBottom: '5px' }}>
                {inc.from} ↔ {inc.to} — {inc.type === 'closure' ? 'Closure' : 'Accident'}
                <button 
                  onClick={() => handleClearIncident(inc.from, inc.to)}
                  style={{ marginLeft: '10px', fontSize: '0.7rem', padding: '2px 5px', cursor: 'pointer' }}
                >
                  Clear
                </button>
              </li>
            ))}
          </ul>
        </div>
      )}
    </div>
  );
}
