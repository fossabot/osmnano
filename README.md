[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2FJeremyGrosser%2Fosmnano.svg?type=shield)](https://app.fossa.io/projects/git%2Bgithub.com%2FJeremyGrosser%2Fosmnano?ref=badge_shield)

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
MIT. Third party dependencies may vary. See [LICENSE](../master/LICENSE)
for details.



## License
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2FJeremyGrosser%2Fosmnano.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2FJeremyGrosser%2Fosmnano?ref=badge_large)