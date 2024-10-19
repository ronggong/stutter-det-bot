# Zoom Meeting SDK for Windows Raw Data Demos


TO download the repo
`git clone https://github.com/tanchunsiong/MSDK_RawDataDemos.git`

Thereafter launch `StutterDetBot.sln` from explorer or open up from Visual Studio.

Within each sample folder you will need 2 addition items
- Create a file named `config.json` in each folder. Here are some parameters which are expected within `config.json`
```
{
  "sdk_key": "",
  "sdk_secret": "",
  "meeting_number": "123123123123",
  "passcode": "123123",
  "zak": ""
}
```

- In each sample folder, populate the folder named "SDK" with the Zoom Meeting SDK libraries. These are intentionally not included in the repo. 
They can be downloaded from the (Zoom SDK website)[marketplace.zoom.us]. The file/folder structure within the SDK folder should be as follows:
	- SDK
		- x64
		- x86
		- CHANGELOG.MD
		- OSS-LICENSE.txt
		- README.md
		- version.txt

