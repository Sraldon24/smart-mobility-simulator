import React from 'react';
import { Play, FastForward, RotateCcw, Car } from 'lucide-react';

export default function SimulationControls({
  onStep,
  onReset,
  onSpawnBatch
}) {
  return (
    <div className="flex-col gap-md">
      <h2>Simulation</h2>

      <div className="flex-col gap-sm">
        <label className="text-secondary text-sm">Spawn Random Vehicles</label>
        <div className="flex-row gap-sm">
          <button className="btn" style={{ flex: 1 }} onClick={() => onSpawnBatch(10)}>+10</button>
          <button className="btn" style={{ flex: 1 }} onClick={() => onSpawnBatch(50)}>+50</button>
          <button className="btn" style={{ flex: 1 }} onClick={() => onSpawnBatch(100)}>+100</button>
        </div>
      </div>

      <div className="flex-col gap-sm" style={{ marginTop: '0.5rem' }}>
        <label className="text-secondary text-sm">Time Controls</label>
        <div className="flex-row gap-sm">
          <button className="btn btn-primary" style={{ flex: 2 }} onClick={() => onStep(1.0)}>
            <Play size={16} /> +1s
          </button>
          <button className="btn" style={{ flex: 2 }} onClick={() => onStep(5.0)}>
            <FastForward size={16} /> +5s
          </button>
          <button className="btn" style={{ flex: 1, padding: '0.5rem' }} onClick={onReset} title="Reset Simulation">
            <RotateCcw size={16} />
          </button>
        </div>
      </div>
    </div>
  );
}

