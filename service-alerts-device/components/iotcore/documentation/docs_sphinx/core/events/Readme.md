# IoT Core Events
Events lib will be the core way of comm
3 specific event loops  will be initialized
1. Device Info 
    This will contain events that will tell the app about state of application or system.

2. Device error
    This will contain error messages critical to system working. For now it will contain everything that requires handling of system errorhandling implementation.

3. Device data
    This will contain events that will help the application in communicating through interfaces. First implementation will be allowing users to communicate through mqtt.



All event posts require a timeout to be specified to wait for event to be published. Will be selecting this to be 100ms.