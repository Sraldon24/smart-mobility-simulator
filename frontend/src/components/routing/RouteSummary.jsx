import { ArrowUpRight, Clock, Route } from "lucide-react";
export default function RouteSummary({ routeResult, routeError }) {
  if (routeError)
    return (
      <div className="notice error" role="alert">
        {routeError}
      </div>
    );
  if (!routeResult) return null;
  if (!routeResult.found)
    return (
      <div className="notice">
        No connected route here. Try points on another street.
      </div>
    );
  return (
    <section
      className="route-result"
      aria-label="Route summary"
      aria-live="polite"
    >
      <div className="flex-row gap-sm">
        <ArrowUpRight size={18} />
        <strong>Your route is ready</strong>
      </div>
      <div className="journey-stats">
        <div>
          <Clock size={17} />
          <strong>
            {(routeResult.estimatedTravelTimeSeconds / 60).toFixed(1)}
            <small> min</small>
          </strong>
        </div>
        <div>
          <Route size={17} />
          <strong>
            {(routeResult.totalDistanceMeters / 1000).toFixed(2)}
            <small> km</small>
          </strong>
        </div>
      </div>
      <p>Follow the highlighted route on the map.</p>
      {Number.isFinite(routeResult.runtimeMicroseconds) && (
        <details>
          <summary>Calculation details</summary>
          <p>
            {routeResult.algorithm} · {routeResult.nodesExplored} intersections
            explored · {(routeResult.runtimeMicroseconds / 1000).toFixed(2)} ms
          </p>
        </details>
      )}
    </section>
  );
}
