# Iris-RESTful-Server

Copyright &copy; 2025 Iris Developers; MIT Software License

The Iris RESTful server provides performance access to Iris encoded slide tile data for viewer implementations that use HTTP networking protocols. The server internally uses the [Boost Beast / ASIO]() library for networking protocols and the [Iris File Extension (IFE)]() library for slide tile access.

This server is extremely light-weight, available as a 10 MB docker image, but can support >3000 slide tile requests per second with <35 ms median response times under that request load (tested with [Locust](https://locust.io) on a single Rockchip RK3588 [Turing RK1](https://turingpi.com/product/turing-rk1/?attribute_ram=8+GB) based server implementation).

A corresponding derived [OpenSeaDragon](https://openseadragon.github.io) TileSource, the [IrisTileSource](), has been developed to allow for JavaScript slide viewer applications to immediately begin taking advantage of the Iris File Extension slide format. 

> [!WARNING]
> The Iris RESTful Server is still in active development. Only WADO-RS (DICOMweb) like GET calls are supported at this time. Please check in regularly for updates to the API if you plan to update your implementation.

> [!NOTE]
>  The Iris RESTful Server is **NOT** intended as an image management system (IMS). This server complements existing IMS implementations by providing fast tile data and slide metadata as a drop-in replacement for Deep Zoom Images (DZI). A proper system architecture should have an IMS return the slide handle (file name) and then have the whole slide viewer access that file from the Iris RESTful Server.

> [!NOTE]
>  The Iris RESTful Server is **NOT** the same implementation as our internal binary networking routines. It is provided to allow for IFE compatability with HTTP (mostly JavaScript) viewer platforms such as those based on OpenSeaDragon. It is very efficient but does not attempt to achieve the same performance as our UDP based binary transport protocols. 

### API Introduction
Iris RESTful has a simple API, outlined here, and explained in greater detail within the [API Exlained Section](README.md#api-explained)
```
Iris RESTful
GET <URL>/slides/<slide-name>/metadata
GET <URL>/slides/<slide-name>/layers/<layer>/tiles/<tile>

Supported WADO-RS
GET <URL>/studies/<study>/series/<UID>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/metadata
GET <URL>/studies/<study>/series/<UID>/instances/<layer>/frames/<tile>
```
### Deployment Introduction
Deploying an IrisRESTful Server is extremely simple. We recommend container deployment, but we describe the different methods for deploying a slide server in the [Deployment Section](README.md#deployment). The following is an out of the box deployment with `${SLIDES_DIRECTORY}` aliasing a directory that will be mounted to the container and contains the Iris slide files and `${CONNECTION_PORT}` describing the port that the container will use to listen. 
```sh
docker run --rm -v${SLIDES_DIRECTORY}:/slides -p ${CONNECTION_PORT}:3000 ghcr.io/iris-digital-pathology/iris-restful
```

# API Explained

The Iris RESTful API was designed to match the [DICOMweb](https://www.dicomstandard.org/using/dicomweb) (presently only the [WADO-RS](https://www.dicomstandard.org/using/dicomweb/retrieve-wado-rs-and-wado-uri)) API as a means of replacing the Deep Zoom Image reliance in OpenSeaDragon whole slide viewer implementations used in Digital Pathology. The Iris RESTful server supports 2 API:
- Iris RESTful 
- DICOMweb (WADO-RS)

The specific API you plan to use will be indicated up front. Presently the API was intentionally limited to the HTTP URL target sequence for DICOMweb compatability. The entry points are as follows:
- Iris RESTful API: `<URL>/slides/`
- WADO-RS API: `<URL>/studies/`


## Retrieve Metadata
Metadata is returned in the form of a JSON object with the following structure
```json
{
    "type": "iris_metadata",       // Confirm Slide Metadata
    "format": "FORMAT_R8G8B8A8",    // Source byte format
    "encoding": "image/jpeg",       // Tile Encoding Format
    "extent": {
        "width": 1983,              // Layer 0 window (pixels)
        "height": 1381,             // Layer 0 window (pixels)
        "layers": [                 // Array of layer extents
            {                       // IFE 256 pixel tiles
                "x_tiles": 8,       // Layer 0
                "y_tiles": 6,
                "scale": 1.0
            },
            {
                "x_tiles": 31,      // Layer 1
                "y_tiles": 22,
                "scale": 4.0
            },
            {
                "x_tiles": 124,     // Layer 2
                "y_tiles": 87,
                "scale": 16.0
            },
            {
                "x_tiles": 496,     // Layer 3
                "y_tiles": 346,
                "scale": 64.0
            }
        ]
    },
    "attributes" : {                // List of slie attributes
        "aperio.ScannerType" : "GT450"
        // Any additional attributes
    },
    "associated_images": [          // List of non-tile images
        "thumbnail",
        "tabel",
    ],
}
```
### Iris RESTful
```
GET <URL>/slides/<slide-name>/metadata 
```
### WADO-RS (supported calls)
```
GET <URL>/studies/<study>/series/<UID>/metadata 
GET <URL>/studies/<study>/series/<UID>/instances/<layer-number>/metadata
```
It is important to note that Iris File Extension encodes the entire slide in a single file. It does **not** represent layers as individual files with duplicated metadata like native DICOM. Therefore in IFE encode files, there is only a single authoritative version of the metadata and thus metadata GET requests for a single layer / DICOM-instance (*code-block line 2*) returns only some metadata when called. It is preferred that viewers simply call the entire slide metadata (*line 1*), which contains an array of layer specific information as well. 

> [!WARNING]
> THIS SECTION IS INCOMPLETE

# Deployment

> [!WARNING]
> THIS SECTION IS INCOMPLETE

For best performance, we direct use of network host mode to avoid the overhead of docker-proxy, which at times can be substantial. 
```sh
docker run --rm -v${SLIDES_DIRECTORY}:/slides --network host ghcr.io/iris-digital-pathology/iris-restful
```