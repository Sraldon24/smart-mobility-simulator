# Downtown Montréal demo data

`downtown.osm.pbf` is a deliberately small, versioned OpenStreetMap extract used by the deployed simulator. It covers downtown Montréal, including Concordia, McGill and Old Montréal. It is not the entire metropolitan area.

- Source: local Montréal extract from [BBBike downloads](https://download.bbbike.org/), as identified by its PBF header.
- Source snapshot: 2026-10-02 23:00 UTC.
- Requested bounds: west -73.59, south 45.485, east -73.55, north 45.525.
- Complete roads may extend slightly beyond these bounds.
- Size: 459,191 bytes; 23,004 OSM nodes, 5,141 drivable ways, no relations.
- SHA-256: `3ca33cdd95aeb0fa1128d1bc7eca7830c3f4d5ef3800241a2ae2723c9422353f`.
- © [OpenStreetMap contributors](https://www.openstreetmap.org/copyright), available under the [Open Database License 1.0](https://opendatacommons.org/licenses/odbl/1-0/).

The original `montreal.osm.pbf` and generated `.bin` caches remain ignored. Only this explicit downtown extract is included in Docker builds. Override `OSM_PBF_PATH` to use another compatible local extract.

Reproduce with [osmium-tool](https://osmcode.org/osmium-tool/):

```sh
osmium extract --bbox=-73.59,45.485,-73.55,45.525 --strategy=complete_ways \
  -S relations=false --set-bounds montreal.osm.pbf -o downtown-all.osm.pbf
osmium tags-filter downtown-all.osm.pbf \
  w/highway=motorway,trunk,primary,secondary,tertiary,residential,unclassified,service,motorway_link,trunk_link,primary_link,secondary_link,tertiary_link \
  -o downtown.osm.pbf
osmium check-refs downtown.osm.pbf
```

Reference validation: zero missing way-node references.
