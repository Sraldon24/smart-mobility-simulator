import React from "react";
import { Star, BarChart2 } from "lucide-react";

export default function RecommendationPanel({
  recommendationResult,
  selectedProfile,
  setSelectedProfile,
  onRecommend,
  onCandidateSelect,
  disabled,
}) {
  const profiles = [
    { id: "balanced", label: "Balanced" },
    { id: "fastest", label: "Fastest" },
    { id: "shortest", label: "Shortest" },
    { id: "least_traffic", label: "Least Traffic" },
    { id: "cheapest", label: "Cheapest" },
  ];

  return (
    <div className="flex-col gap-md">
      <h2>Recommendations</h2>

      <div className="flex-col gap-sm">
        <label className="text-secondary text-sm">User Profile</label>
        <select
          value={selectedProfile}
          onChange={(e) => setSelectedProfile(e.target.value)}
        >
          {profiles.map((p) => (
            <option key={p.id} value={p.id}>
              {p.label}
            </option>
          ))}
        </select>
      </div>

      <button
        className="btn btn-primary"
        onClick={onRecommend}
        disabled={disabled}
      >
        <Star size={16} /> Get Recommendation
      </button>

      {recommendationResult && (
        <div className="flex-col gap-sm" style={{ marginTop: "0.5rem" }}>
          <div
            className="text-sm text-secondary"
            style={{ fontStyle: "italic" }}
          >
            {recommendationResult.explanation}
          </div>

          <div className="flex-col gap-sm">
            {recommendationResult.candidates.map((cand, idx) => {
              const isRecommended =
                cand.score === recommendationResult.recommendedRoute.score &&
                cand.distanceMeters ===
                  recommendationResult.recommendedRoute.distanceMeters;
              return (
                <div
                  key={idx}
                  className={`card interactive ${isRecommended ? "active" : ""}`}
                  onClick={() =>
                    onCandidateSelect({
                      found: true,
                      nodeIds: cand.nodeIds,
                      totalDistanceMeters: cand.distanceMeters,
                      estimatedTravelTimeSeconds: cand.travelTimeSeconds,
                      algorithm: cand.sourceObjective,
                      objective: cand.sourceObjective,
                    })
                  }
                >
                  <div
                    className="flex-row"
                    style={{
                      justifyContent: "space-between",
                      marginBottom: "0.5rem",
                    }}
                  >
                    <span
                      className="font-bold"
                      style={{ textTransform: "capitalize" }}
                    >
                      {cand.sourceObjective.replace("_", " ")} Route
                    </span>
                    {isRecommended && (
                      <span
                        className="text-sm"
                        style={{ color: "var(--accent)", fontWeight: 600 }}
                      >
                        ★ Recommended
                      </span>
                    )}
                  </div>

                  <div
                    style={{
                      display: "grid",
                      gridTemplateColumns: "1fr 1fr",
                      gap: "0.25rem",
                      fontSize: "0.8rem",
                      color: "var(--text-secondary)",
                    }}
                  >
                    <span>{(cand.distanceMeters / 1000).toFixed(2)} km</span>
                    <span>{(cand.travelTimeSeconds / 60).toFixed(1)} min</span>
                    <span>{cand.averageCongestion.toFixed(2)}x traffic</span>
                    <span>Score: {cand.score.toFixed(3)}</span>
                  </div>

                  {/* Score Breakdown Bars */}
                  {isRecommended && (
                    <div
                      className="flex-col gap-sm"
                      style={{
                        marginTop: "0.75rem",
                        borderTop: "1px solid var(--border)",
                        paddingTop: "0.5rem",
                      }}
                    >
                      <div className="flex-row gap-sm text-sm">
                        <BarChart2 size={14} className="text-muted" /> Score
                        Breakdown
                      </div>
                      {[
                        {
                          label: "Time",
                          val: cand.scoreBreakdown.time,
                          color: "#388bfd",
                        },
                        {
                          label: "Dist",
                          val: cand.scoreBreakdown.distance,
                          color: "#238636",
                        },
                        {
                          label: "Traf",
                          val: cand.scoreBreakdown.traffic,
                          color: "#d29922",
                        },
                        {
                          label: "Cost",
                          val: cand.scoreBreakdown.cost,
                          color: "#8957e5",
                        },
                      ].map(
                        (f) =>
                          f.val > 0.001 && (
                            <div
                              key={f.label}
                              className="flex-row gap-sm"
                              style={{ fontSize: "0.75rem" }}
                            >
                              <span
                                style={{
                                  width: "30px",
                                  color: "var(--text-muted)",
                                }}
                              >
                                {f.label}
                              </span>
                              <div
                                className="progress-bar-bg"
                                style={{ flex: 1, marginTop: 0 }}
                              >
                                <div
                                  className="progress-bar-fill"
                                  style={{
                                    width: `${Math.min(100, f.val * 100)}%`,
                                    backgroundColor: f.color,
                                  }}
                                ></div>
                              </div>
                              <span
                                style={{ width: "35px", textAlign: "right" }}
                              >
                                {f.val.toFixed(2)}
                              </span>
                            </div>
                          ),
                      )}
                    </div>
                  )}
                </div>
              );
            })}
          </div>
        </div>
      )}
    </div>
  );
}
