import React from 'react';
import { Activity, Clock } from 'lucide-react';

export default function VehicleMetrics({
  simulationTime,
  congestedRoadCount,
  averageCongestion,
  totalReroutes,
  vehicles
}) {
  const formatTime = (seconds) => {
    const m = Math.floor(seconds / 60);
    const s = Math.floor(seconds % 60);
    return `${m}:${s.toString().padStart(2, '0')}`;
  };

  const waiting = vehicles.filter(v => v.state === 'waiting').length;
  const moving = vehicles.filter(v => v.state === 'moving').length;
  const arrived = vehicles.filter(v => v.state === 'arrived').length;

  return (
    <div className="card flex-col gap-sm">
      <div className="flex-row" style={{ justifyContent: 'space-between', marginBottom: '0.25rem' }}>
        <h3 className="flex-row gap-sm"><Clock size={16}/> {formatTime(simulationTime)}</h3>
        <span className="text-secondary text-sm">Total: {vehicles.length}</span>
      </div>

      <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr', gap: '0.5rem', textAlign: 'center' }}>
        <div className="flex-col">
          <span className="text-sm" style={{ color: 'var(--warning)' }}>{waiting}</span>
          <span className="text-muted" style={{ fontSize: '0.75rem' }}>WAITING</span>
        </div>
        <div className="flex-col">
          <span className="text-sm" style={{ color: 'var(--accent)' }}>{moving}</span>
          <span className="text-muted" style={{ fontSize: '0.75rem' }}>MOVING</span>
        </div>
        <div className="flex-col">
          <span className="text-sm" style={{ color: 'var(--success)' }}>{arrived}</span>
          <span className="text-muted" style={{ fontSize: '0.75rem' }}>ARRIVED</span>
        </div>
      </div>

      <div style={{ borderTop: '1px solid var(--border)', marginTop: '0.5rem', paddingTop: '0.5rem', display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '0.5rem' }}>
        <div className="flex-row gap-sm text-sm" title="Congested Roads">
          <Activity size={14} className={congestedRoadCount > 0 ? "text-danger" : "text-muted"} />
          <span>{congestedRoadCount} congested ({averageCongestion.toFixed(2)}x)</span>
        </div>
        <div className="text-sm text-muted" style={{ textAlign: 'right' }}>
          {totalReroutes} reroutes
        </div>
      </div>
    </div>
  );
}

