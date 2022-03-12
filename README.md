# dump978 #

Experimental demodulator/decoder for 978MHz UAT signals.


## A note about future development ##

I'm in Europe which doesn't use UAT, so there won't be much spontaneous
development going on now that the demodulator is at a basic "it works" stage.

I'm happy to look at signal or message captures and help with further
development, but it really needs to be driven by whoever is actually using the
code to receive UAT!


## Demodulator ##

`dump978` is the demodulator. It expects 8-bit I/Q samples on `stdin` at **2.083334MHz**, for example:

```
$ rtl_sdr -f 978000000 -s 2083334 -g 48 - | ./dump978
```

It outputs one one line per demodulated message, in the form:

```
+012345678..; this is an uplink message
-012345678..; this is a downlink message
```

**For parsers:** ignore everything between the first semicolon and newline that you don't understand, it will be used
for metadata later. See reader.[ch] for a reference implementation.


## Decoder ##

To decode messages into a readable form, use `uat2text`:

```
$ rtl_sdr -f 978000000 -s 2083334 -g 48 - | ./dump978 | ./uat2text
```


## Sample data ##

Around 1,100 sample messages are in the file `sample-data.txt.gz`.

They are the output of the demodulator from various RF captures I have on hand. This file can be fed to `uat2text` _(and
related utilities)_:

```
$ zcat sample-data.txt.gz | ./uat2text
```

When testing, this is much easier on your CPU (and disk space!) than starting from the raw RF captures.


## Filtering for just uplink or downlink messages ##

As the uplink and downlink messages start with different characters, you can filter for just one type of message very
easily with `grep`:


### Uplink Messages Only ###

```
$ zcat sample-data.txt.gz | grep "^+" | ./uat2text
```


### Downlink Messages Only ###

```
$ zcat sample-data.txt.gz | grep "^-" | ./uat2text
```


## Map generation via `uat2json` ##

`uat2json` writes `aircraft.json` files in the format expected by [my fork of dump1090][1]'s map html/javascript.


### Setup of a Live Map Feed ###

To set up a live map feed:

1) Get a copy of [this dump1090 repository][1]; we're going to reuse its mapping **html/javascript** resources:

```
$ git clone https://github.com/mutability/dump1090.git dump1090-dump978-web
```

2) Create directories and set permissions for the **html/javascript** somewhere where your webserver can reach, such as
   within the `/var/www` or `/srv/http` path:

```
$ (sudo) mkdir /path/to/dump978map
$ (sudo) chgrp -R www-data /path/to/dump978map
$ (sudo) chmod -R 2775 /path/to/dump978map
```

3) Copy the **html/javascript** resources to the newly created directories:

```
$ cp -ra /path/to/dump1090-dump978-web/public_html/* /path/to/dump978map/
```
or
```
$ rsync -aAXv /path/to/dump1090-dump978-web/public_html/ /path/to/dump978map
```

4) Create an empty `data` subdirectory

```
$ (sudo) mkdir /path/to/dump978map/data
```

5) Ensure permissions are correctly configured for `/path/to/dump978map`:

```
$ (sudo) chgrp -R www-data /path/to/dump978map
$ (sudo) chmod -R 2775 /path/to/dump978map
```

6) Feed `uat2json` from `dump978`:

```
$ rtl_sdr -f 978000000 -s 2083334 -g 48 - | ./dump978 | ./uat2json /var/www/dump978map/data
```

7) Go look at [http://localhost/dump978map/][2]


## `uat2esnt`: convert UAT ADS-B messages to Mode S ADS-B messages. ##

**Warning: _This one is particularly experimental._**

`uat2esnt` _accepts_ **978MHz UAT downlink messages** on `stdin` and _generates_ **1090MHz Extended Squitter messages**
on `stdout`.

The generated messages mostly use `DF18` with `CF=6`, which is for _rebroadcasts_ of ADS-B messages _(ADS-R)_.

The output format is the **"AVR"** text format; this can be fed to `dump1090` on **port 30001** by default. Other ADS-B
tools may accept it too -- e.g. [VRS][3] seems to accept _most_ of it _(though it ignores `DF18` `CF=5` messages, which
are generated for non-ICAO-address callsign/squawk information)_.


### Example Pipelines ###


#### Listening for incoming connections ####

To _listen_ for incoming connections, allowing other _aviation-related_ software to connect to the output of `dump978`
and `uat2esnt`, use the following command line:

```
$ rtl_sdr -f 978000000 -s 2083334 -g 48 - | ./dump978 | ./uat2esnt | nc -q1 localhost 30001
```


#### Pushing messages to an external server ####

To _push_ messages to an external server, such as [VRS][3] or [readsb][4] _(running on the same host, or perhaps a host
on the local network, or even the internet)_ that's listening for incoming messages on specific port _(for example, port
**33001** on IP address **10.4.10.35**)_, we would use the following command line:

```
$ rtl_sdr -f 978000000 -s 2083334 -g 48 - | ./dump978 | ./uat2esnt | nc -q1 10.4.10.35 33001
```


[1]:	<https://github.com/mutability/dump1090>
[2]:	<http://localhost/dump978map/>
[3]:	<https://www.virtualradarserver.co.uk>

