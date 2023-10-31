# Creating and Connecting to an Arduino IoT Device

One of the key features of the SparkFun DataLogger IoT is it's simplified access to IoT service providers. This document outlines how an Arduino IoT Device is used by the DataLogger Iot.

The following is covered by this document:

* Structure of the Arduino IoT Cloud devices
* Device creation in the Arduino IoT cloud
* Setup of the Arduino IoT driver
* How data is posted from the Flux Framework to the Arduino IoT Device

Currently, the Arduino IoT device connection is a single direction - used to post data from the DataLogger IoT to an Arduino IoT Device.

## General Operation

The Arduino IoT Cloud enables connectivity between an IoT/Edge Arduino enabled device and the cloud. The edge device updates data in the Arduino Cloud by updating `variables` or parameters attached to a cloud device.

In the Arduino Cloud, the edge device is represented by a *Device* which has a virtual *Thing* attached/associated with it. The *Thing* acts as a container for a list of *parameters* or *variables* which represent the data values received from the edge device. As values on the edge device update, they are transmitted to the Arduino Cloud.

For a SparkFun DataLogger IoT connected to an Arduino IoT device, the output parameters of a device connected to the DataLogger are mapped to variables within the Arduino IoT Devices Thing using a simple pattern of *DeviceName*_*ParameterName* for the name of the variable in the Arduino IoT Cloud.

![Arduino IoT Overview](images/aiot_overview.png)

## Creating a Device in Arduino IoT

The first step connecting to the Arduino IoT cloud is setting up a device within the cloud. A device is a logical element represents a physical device.

The first step is to login to your Arduino IoT cloud account and navigate to the Devices page on the IoT Cloud. This page lists your currently defined devices.

![IOT Cloud](images/aiot_cloud_sel.png)

First select the *Add* button on the top of the page.

![Add a Device](images/aiot_dev_add.png)

A device type selection dialog is then shown. Since we are connecting a DataLogger IoT board to the system, and not connected a known device, select **DIY** - *Any Device*.

![Select DIY Device](images/aiot_dev_setup_sel.png)

Once selected, another dialog is presented. Just select *Continue*.

At this point you can provide a name for your device.

![Name Device](images/aiot_dev_name.png)

The next screen is the critical step of the device creation process. This step is the one time the Device Secret Key is available. The provided ```Device ID``` and Device ```Secret Key``` values are needed to connect to the Arduino IoT Cloud. Once this step is completed, the Secret Key is no longer available.

![Device Secret](images/aiot_dev_secrets.png)

The easiest way to capture these values is by downloading as a PDF file, which is offered on the setup page.  

## Arduino Cloud API Keys

In addition to creating a device, to access the Arduino IoT Cloud, the driver requires a API Key. This allows the Flux Arduino IoT Cloud driver to access the web API of the Arduino Cloud. This API is used to setup the connection to the Arduino Cloud.

To create an API key, start at the **Arduino Cloud** [home page](https://cloud.arduino.cc/home/). From this page, select the *API keys* menu entry on the left side of the page.

![API Keys](images/aiot_cloud_api-k.png)

This menu takes you to a list of existing API Keys. From this page, select the *CREATE API KEY* button, which is on the upper right of the page.

![Create Key](images/aiot_cloud_create_key.png)

In the presented dialog, enter a name for the API key.

![API Key Name](images/aiot_cloud_key_name.png)

Once the name is entered and *CONTINUE* selected, a page with the new API key is presented. Like in Device Creation, this page contains a secret that is only available on this page during this process.

![API KEY NOW](images/aiot_cloud_key_secret.png)

Make note of the *Client ID* and *Client Secret* values on this page. The best method to capture these values is to download the PDF file offered on this page.  

At this point, the Arduino IoT cloud is setup for connection by the driver.

## Adding an Arduino IoT Connection to the DataLogger

To add an Arduino IoT device as a destination DataLogger IoT, the Arduino IoT connection is enabled via the DataLogger menu system and the connection values, obtained from the Arduino IoT Cloud (see above) are set in the connection properties.

![Menu Entries](images/aiot_dl_menu.png)

### Device ID and Secret

These values are used to identify the Arduino IoT device that is connected to. These are obtained via the steps outlined earlier in this document.

### API ID and Secret

These values are used to provide API access by the driver. This access allows for the creation/use of a Thing and Variables within the Arduino IoT Cloud. These are obtained via the steps outlined earlier in this document.

### Thing Name

The name of the Arduino Iot Cloud ```Thing``` to use. If the Thing doesn't exist on startup, the driver will create the a Thing of this name.

### Thing ID

This is the ID of the Thing being used. This value is obtained by the following methods:

* If the driver creates a new Thing, the ID is obtained and used.
* If an existing Thing is connected to, the driver retrieves it's ID. NOTE: In this case, the driver cannot create any new variables until the system is restarted.
* The user creates a new Thing using the web interface of Arduino IoT Cloud, and provides the *Thing ID* and *Thing Name*.

## Setting Properties

The above property values must be set in the DataLogger Arduino IoT driver before use. They can be set via the settings menu, or via a JSON file that is loaded by the system at startup. For the DataLogger Arduino IoT example outlined in this document, the entries in the settings JSON file are as follows:

```json
"Arduino IoT": {
    "Enabled": true,
    "Thing Name": "SparkFunThing1",
    "API Client ID": "API ID",
    "API Secret": "My API Secrete",
    "Device Secret": "My Device Secret",
    "Device ID": "My Device ID"            
  },
```

## Operation

Once the DataLogger IoT device is configured and running, updates in Arduino IoT are listed in the ***Things*** tab of the Arduino IoT page. Clicking the target Thing provides access to the current variable values.

![Cloud Variables](images/aiot_cloud_vars.png)

### Create a Dashboard

With the data now available in the Arduino IoT Cloud as variables, its a simple step create a dashboard that plots the data values.

The general steps to create a simple dashboard include:

* Select the ***Dashboards*** section of the Arduino IoT Cloud
* Select the ***Create*** button
* Add an element to the dashboard -- for this example select ***Chart**
* On the Chart Widget settings select ***Link Variables**
* The DataLogger Variables are listed - select the variable to link.
* Continue this step until all the desired variables are linked to the chart.

![Linked Variables](images/aiot_dashboard_link.png)

* When all variables are linked, select done.

The created dashboard then displays the values posted from the SparkFun DataLogger IoT!

![DataLogger Dashboard](images/aiot_dashboard.png)
