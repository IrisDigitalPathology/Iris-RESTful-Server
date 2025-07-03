# Iris-RESTful-Server

Copyright &copy; 2025 Ryan Landvater and Iris Developers; MIT Software License

The Iris RESTful server provides performance access to Iris encoded slide tile data for viewer implementations that use HTTPS networking protocols. The server internally uses the [Boost Beast / ASIO](https://github.com/boostorg/beast) library for networking protocols and the [Iris File Extension (IFE)](https://github.com/IrisDigitalPathology/Iris-File-Extension) library for slide tile access.

This server is extremely light-weight, available as a < 20 MB docker image, but can support >7500 slide tile requests per second with <35 ms median response times under that request load (tested with [Locust](https://locust.io) on a single 200$ Rockchip RK3588 [Turing RK1](https://turingpi.com/product/turing-rk1/?attribute_ram=8+GB) based server implementation).

A corresponding derived [OpenSeaDragon](https://openseadragon.github.io) TileSource, the [IrisTileSource]() (authored by [Navin Kathawa](https://github.com/nkathawa)), has been developed to allow for JavaScript slide viewer applications <u>to immediately begin taking advantage of the Iris File Extension slide format in existing slide viewer implementations</u>. 

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
Deploying an IrisRESTful Server is extremely simple. We recommend container deployment, but we describe the different methods for hosting a slide server in the [Deployment Section](README.md#deployment). The following is a one-line deployment with `${SLIDES_DIRECTORY}` aliasing a directory that will be mounted to the container and contains the Iris slide files and `${CONNECTION_PORT}` describing the port that the container will use to listen. 
```sh
docker run --rm -v${SLIDES_DIRECTORY}:/slides -p ${CONNECTION_PORT}:3000 ghcr.io/iris-digital-pathology/iris-restful:latest
```
An example [Docker Compose YAML](./Docker/compose.yaml) is also provided for ease in deployment.

>[!NOTE]
> IrisRESTful may be *optionally* configured  as a secure webserver for your viewer implementation. We do not recommend this architecture, and instead prefer a more modular microservice design; however operating as a webserver is a supported feature. If you are not using this optional webserver feature and instead are deploying IrisRESTful in a more modular capacity (as we suggest) you **must** enable **cross origin resource sharing (CORS)** to allow access to the Iris slides. See the below [Deployment Section](README.md#deployment) for more information on CORS.

# API Explained

The Iris RESTful API was designed to match the [DICOMweb](https://www.dicomstandard.org/using/dicomweb) (presently only the [WADO-RS](https://www.dicomstandard.org/using/dicomweb/retrieve-wado-rs-and-wado-uri)) API as a means of replacing the Deep Zoom Image reliance in OpenSeaDragon whole slide viewer implementations used in Digital Pathology. The Iris RESTful server supports 2 API:
- Iris RESTful 
- DICOMweb (WADO-RS)

The specific API you plan to use will be indicated up front. Presently the API was intentionally limited to the HTTP URL target sequence for DICOMweb compatability. The entry points are as follows:
- Iris RESTful API: `<URL>/slides/`
- WADO-RS API: `<URL>/studies/`


## Retrieve Metadata
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
### Metadata Structure
Metadata is returned in the form of a JSON object with the structure shown in the below example.
```json
{
    "type": "iris_metadata",      
    "format": "FORMAT_R8G8B8A8",    
    "encoding": "image/jpeg",      
    "extent": {
        "width": 1983,              
        "height": 1381,             
        "layers": [                 
            {                       
                "x_tiles": 8,      
                "y_tiles": 6,
                "scale": 1.0
            },
            {
                "x_tiles": 31,      
                "y_tiles": 22,
                "scale": 4.0
            },
            {
                "x_tiles": 124,     
                "y_tiles": 87,
                "scale": 16.0
            },
            {
                "x_tiles": 496,     
                "y_tiles": 346,
                "scale": 64.0
            }
        ]
    },
    "attributes" : {                
        "aperio.ScannerType" : "GT450"
    },
    "associated_images": [          
        "thumbnail",
        "tabel",
    ],
}
```

> [!WARNING]
> THIS SECTION IS INCOMPLETE

# Deployment
IrisRESTful may be depolyed as a containerized implementation or may be natively run on your hardware. We **strongly suggest** deploying IrisRESTful as a container rather than running it natively. The container can be built from source or pulled from our [container repository on Github (GHCR)](ghcr.io/iris-digital-pathology/iris-restful). If you wish to build from source, please use our CMakeList.txt scripts as CMake is our only supported build system. 

Iris RESTful is run with the following arguments:\
**Arugments:**
 - **-h** *or* **--help**: Print the help text
 - **-p** *or* **--port**: Port from which the server will listen for connections
 - **-d** *or* **--dir**: Directory path to the directory containing the Iris Slide Files to be served
 - **-c** *or* **--cert**: *(optional)* Public SSL certificate in PEM format for establishing HTTPS connections
 - **-k** *or* **--key**: *(optional)* Private key in PEM format to sign argument provided in CERT
 - **-o** *or* **--cors**: *(optional)* Slide viewer domain. Returned in 'Access-Control-Allow-Origin' header
 - **-r** *or* **--root**: *(optional)* Web viewer server document root directory.

 The use of CORS and root are generally mutally exclusive, as a web viewer server  should not need to return Access-Control-Allow-Origin responses because is serving up its own slide files.
```sh
IrisRESTful -d <slide-dir> -p <exposed_port> -c <path-to-cert> -k <path-to-key> -o <viewer-domain>
```
Example: 
```sh
IrisRESTful -d /slides -p 3000 -c /etc/ssh/cert.pem -k /ect/ssh/private/key.pem -o slide-viewer.com
```

This implementation works with the provided container by overloading the default CMD arguments (`IrisRESTful` is the entry point)
```sh
docker run --rm -v${SLIDES_DIRECTORY}:/slides -v${CERT_ROOT}:/ect/ssh -p ${CONNECTION_PORT}:3000 ghcr.io/iris-digital-pathology/iris-restful:latest -d /slides -p 3000 -c /etc/ssh/cert.pem -k /ect/ssh/private/key.pem -o slide-viewer.com
```

IrisRESTful can be deployed in two modes: 
1) **MODULAR CORS**: As a modular slide server ONLY (and will respond ONLY to the above [API](README.md/#api-explained)):
    - This model requires enabling <u><b>[cross origin resource sharing (CORS)](https://en.wikipedia.org/wiki/Cross-origin_resource_sharing#:~:text=CORS%20defines%20a%20way%20in,allowing%20all%20cross%2Dorigin%20requests)</b></u> within your JS/HTML to allow the viewer to access to the Iris slides on a separate connection from the web-service providing your viewer instance.
    Here is an example of how to do this with OpenSeaDragon:
        ```js
        // Example OpenSeaDragon IrisTileSource with CORS enabled
        tileSources: new OpenSeadragon.IrisTileSource({
            serverUrl: "<server_url>", 
            slideId: "slide_name",
            crossOriginPolicy: 'Anonymous', // CORS ENABLED
        }),
        ```
    - As such you **should** (but are not obligated) to provide IrisRESTful with your web viewer's domain.
        ```sh
        docker run --rm -p<>:3000 -v<>:/slides docker run --rm -v${SLIDES_DIRECTORY}:/slides -p ${CONNECTION_PORT}:3000 ghcr.io/iris-digital-pathology/iris-restful:latest -d /slides -p 3000 --cors ${YOUR_DOMAIN.COM}
        ```
2) **WEBVIEWER SLIDE-SERVER**: As a webserver with slide serving capabilities by providing the document root ('doc-root') containing your viewer webpage.

> [!WARNING]
> THIS SECTION IS INCOMPLETE