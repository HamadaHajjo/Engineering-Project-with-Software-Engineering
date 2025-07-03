#include <SPI.h>
#include <WiFi.h>


enum SearchTeamState {
    SEARCH_IDLE,
    SEARCHING,
    CALL_RESCUE_TEAM,
    CALL_SUPPORT,
    EXTINGUISH_FIRE,
    SEARCH_REASSIGNMENT
};

enum SupportTeamState {
    SUPPORT_IDLE,
    WAIT_FOR_CALL,
    LOCATE_SUPPORT_CALL,
    DELIVER_EQUIPMENT,
    SUPPORT_REASSIGNMENT
};

enum RescueTeamState {
    RESCUE_IDLE,
    WAIT_FOR_RESCUE_CALL,
    LOCATE_RESCUE_CALL,
    RESCUE_SURVIVORS
};

//for test purpose
bool hasSearched = false;
bool recievedSupportCall = true;
bool recievedRescueCall = true;

SearchTeamState searchTeamState = SEARCH_IDLE;
SupportTeamState supportTeamState = SUPPORT_IDLE;
RescueTeamState rescueTeamState = RESCUE_IDLE;

void moveSearchFireman(int startX, int startY, int targetX, int targetY) {
    int currentX = startX;
    int currentY = startY;

    // Keep moving until the fireman reaches the target
    while (currentX != targetX || currentY != targetY) {
        // Move horizontally towards the target column
        if (currentY < targetY) {
            currentY++;  // Move right
        } else if (currentY > targetY) {
            currentY--;  // Move left
        }
        // Move vertically towards the target row
        else if (currentX < targetX) {
            currentX++;  // Move down
        } else if (currentX > targetX) {
            currentX--;  // Move up
        }

        // Print the fireman's new position
        printf("Fireman moved to position: (%d, %d)\n", currentX, currentY);

        delay(1000);  // Wait for 1 second
    }

    printf("Fireman has reached the target position: (%d, %d)!\n", targetX, targetY);
}

void moveSupportFireman(int targetX, int targetY) {
    int currentX = 0;
    int currentY = 0;

    // Keep moving until the fireman reaches the target
    while (currentX != targetX || currentY != targetY) {
        // Move horizontally towards the target column
        if (currentY < targetY) {
            currentY++;  // Move right
        } else if (currentY > targetY) {
            currentY--;  // Move left
        }
        // Move vertically towards the target row
        else if (currentX < targetX) {
            currentX++;  // Move down
        } else if (currentX > targetX) {
            currentX--;  // Move up
        }

        // Print the fireman's new position
        printf("Fireman moved to position: (%d, %d)\n", currentX, currentY);

        delay(2000);  // Wait for 2 second
    }

    printf("Suuport Fireman has reached the target position: (%d, %d)!\n", targetX, targetY);
    printf("Helping extinquish fire...");
    delay(5000);
    printf("Fire extinquished, returning...");

    while (currentX != 0 || currentY != 0) {
        if (currentX > 0) {
            currentX--;  // Move left
        } else if (currentY > 0) {
            currentY--;  // Move up
        }
        printf("Fireman moved back to position: (%d, %d)\n", currentX, currentY);
        delay(2000); // 2 sec
    }
}

void moveRescueFireman(int targetX, int targetY) {
    int currentX = 0;
    int currentY = 0;

    // Keep moving until the fireman reaches the target
    while (currentX != targetX || currentY != targetY) {
        // Move horizontally towards the target column
        if (currentY < targetY) {
            currentY++;  // Move right
        } else if (currentY > targetY) {
            currentY--;  // Move left
        }
        // Move vertically towards the target row
        else if (currentX < targetX) {
            currentX++;  // Move down
        } else if (currentX > targetX) {
            currentX--;  // Move up
        }

        // Print the fireman's new position
        printf("Fireman moved to position: (%d, %d)\n", currentX, currentY);

        delay(1000);  // Wait for 1 second
    }

    printf("Rescue Fireman has reached the target position: (%d, %d)!\n", targetX, targetY);
    printf("checking on the survivors.");
    delay(5000);
    printf("carrying the survivor out of building...");

    while (currentX != 0 || currentY != 0) {
        if (currentX > 0) {
            currentX--;  // Move left
        } else if (currentY > 0) {
            currentY--;  // Move up
        }
        printf("Fireman and survivor moved back to position: (%d, %d)\n", currentX, currentY);
        delay(4000);
    }
}

//transitions
void searchTeam() {
    switch (searchTeamState) {
        case SEARCH_IDLE:
            if (!hasSearched) {
                Serial.println("Search team idle. Preparing to search...");
                searchTeamState = SEARCHING;
            } else {
                Serial.println("Search team idle. Waiting for order...");
                delay(10000);
            } 
            break;
        case SEARCHING:
            Serial.println("Searching...");
            moveSearchFireman(0, 0, 4, 6);
            hasSearched = true;    
            searchTeamState = CALL_SUPPORT;
            break;
        case CALL_RESCUE_TEAM:
            Serial.println("Calling rescue team...");
            recievedRescueCall = true;
            moveSearchFireman(4, 6, 0, 0);
            searchTeamState = SEARCH_IDLE;
            //code to call a rescue team member
            break;
        case CALL_SUPPORT:
            Serial.println("Calling for support...");
            recievedSupportCall = true;
            delay(3000);
            searchTeamState = EXTINGUISH_FIRE;
            break;
        case EXTINGUISH_FIRE:
            Serial.println("Extinguishing fire...");
            delay(5000);
            Serial.println("Fire extinguished. Returning...");
            moveSearchFireman(4, 6, 0, 0);
            searchTeamState = SEARCH_IDLE;
            break;
        case SEARCH_REASSIGNMENT:
            Serial.println("Reassigning...");
            searchTeamState = SEARCH_IDLE;
            break;
    }
}

void supportTeam() {
    switch (supportTeamState) {
        case SUPPORT_IDLE:
            Serial.println("Support team idle.");
            supportTeamState = WAIT_FOR_CALL;  // Example transition
            break;
        case WAIT_FOR_CALL:
            if(!recievedSupportCall) 
            {
              Serial.println("Waiting for support call...");
              delay(1000);
            }
            else
            {
              supportTeamState = LOCATE_SUPPORT_CALL;
            }
            break;
        case LOCATE_SUPPORT_CALL:
            Serial.println("Locating support call...");
            delay(1000);
            supportTeamState = DELIVER_EQUIPMENT;
            break;
        case DELIVER_EQUIPMENT:
            Serial.println("Delivering equipment...");
            moveSupportFireman(4,6);
            supportTeamState = SUPPORT_IDLE;
            break;
        case SUPPORT_REASSIGNMENT:
            Serial.println("Support team reassigning...");
            supportTeamState = SUPPORT_IDLE;
            break;
    }
}

void rescueTeam() {
    switch (rescueTeamState) {
        case RESCUE_IDLE:
            Serial.println("Rescue team idle.");
            rescueTeamState = WAIT_FOR_RESCUE_CALL;  
            break;
        case WAIT_FOR_RESCUE_CALL:
            if(!recievedRescueCall)
            {
              Serial.println("Waiting for rescue call...");
              delay(1000);
            }
            else
            {
              rescueTeamState = LOCATE_RESCUE_CALL;
            }
            break;
        case LOCATE_RESCUE_CALL:
            Serial.println("Locating rescue call...");
            rescueTeamState = RESCUE_SURVIVORS;
            break;
        case RESCUE_SURVIVORS:
            Serial.println("Rescuing survivors...");
            moveRescueFireman(4,6);
            rescueTeamState = RESCUE_IDLE;
            break;
    }
}




//Metod som upprättar anslutning till accesspunkten
// author: Hossein Hosseini

//void connectToNet() {
//  WiFi.begin(ssid, password);
//  while (WiFi.status() != WL_CONNECTED) {
//    delay(1000);
//    Serial.println("Connecting to WiFi...");
//  }
//  Serial.println("Connected to WiFi: " + String(ssid));
//}

//Metod som används för att ansluta till Java servern. 
// author: Hossein Hosseini

//void connectToServer() {
//  while (!client.connected()) {
//    if (client.connect(server_address, server_port)) {
//      Serial.println("Connected to server");
//    } else {
//      Serial.println("Failed to connect to server. Retrying in 5 seconds...");
//      delay(5000);
//    }
//  }
//}


void setup() {
    Serial.begin(9600);

    //connectToNet();
    //connectToServer();
}

void loop() {
    //Uncomment one at the time!
    //searchTeam();
    //supportTeam();
    rescueTeam();
}
