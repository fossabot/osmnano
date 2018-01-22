This is a proof of concept demonstrating parsing of OpenStreetMaps osm.pbf
format files using nanopb. It doesn't do anything useful yet.

Build dependencies
---

    apt-get install build-essential python-protobuf protobuf-compiler


Build
---

    make


Run
---

    ./osmnano path/to/planet.osm.pbf

It's recommended that you start with a subset of the planet for testing. For
example, the Geofabrik regional extracts.  
[http://download.geofabrik.de/](http://download.geofabrik.de/)

License
---
MIT. Third party dependencies may vary. See [LICENSE](../blob/master/LICENSE)
for details.

