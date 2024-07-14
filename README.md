# General information

A nightscout webpage is needed, since that is where the data is fetched from with the API.

A token needs roughly the following permissions to fetch data using the API: `api:*:read`

It is possible to remove the token, if your nightscout is publicly open.
In that case the variable named getFromAPI needs a little tweak. However, using tokens is highly recommended.

It is possible to use an ESP32 instead of ESP8266 (d1-mini). In that case the code needs to be re-written since the libraries used are for the ESP8266.

## The following need to be updated to get the project to run.

### secrets.h

Update secrets.h to match your configuration.

### bg_led_strip.ino

Set `LED_COUNT` to match the length of your led array (ws2182b).

Either update `D1` to match the GPIO number for the D1 pin, or set `LED_PIN` to the pin you've soldered the ws2182b data pin to.

# TODOs

* Double check what libraries are actually needed.
* Should the BG level indications be fetched from nightscout api? (i.e. personalizations determine what ranges to use for colours) - feels like overkill
* Lint and unify what the code is written like. Right now it isn't consistent, because of the short amount of time spent to write it.
* Document how to set things up better. What libraries/boards to you need to install using Arduino IDE?