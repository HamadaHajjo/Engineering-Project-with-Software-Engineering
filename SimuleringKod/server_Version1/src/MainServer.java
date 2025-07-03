import Model.CellStatus;
import Model.GameModel;
import org.json.JSONException;
import org.json.JSONObject;
import view.VisualisationServer;
import Model.GameObject;

import java.io.PrintWriter;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;

public class MainServer {
    private final GameModel gameModel;
    private final LogServerGUI logServer;
    private VisualisationServer visualisationServer;

    // Håll reda på aktiva klienter och deras sockets
    private final Set<String> activeClients = ConcurrentHashMap.newKeySet();
    private final ConcurrentHashMap<String, Socket> clientSockets = new ConcurrentHashMap<>();

    public MainServer(GameModel gameModel, LogServerGUI logServer, VisualisationServer visualisationServer) {
        this.gameModel = gameModel;
        this.logServer = logServer;
        this.visualisationServer = visualisationServer;

        // Starta en bakgrundstråd för att övervaka klientanslutningar
        new Thread(this::monitorClients).start();
    }

    public void setVisualisationServer(VisualisationServer visualisationServer) {
        this.visualisationServer = visualisationServer;
    }

    public void startServer(int port) {
        try (ServerSocket serverSocket = new ServerSocket(port)) {
            logServer.appendLog("Server started on port " + port);

            while (true) {
                Socket clientSocket = serverSocket.accept();
                String clientAddress = clientSocket.getInetAddress().toString();

                activeClients.add(clientAddress);
                clientSockets.put(clientAddress, clientSocket);

                logServer.appendLog("New client connected: " + clientAddress);

                new Thread(new ClientHandler(clientSocket, clientAddress)).start();
            }
        } catch (IOException e) {
            logServer.appendLog("Error: " + e.getMessage());
        }
    }

    private class ClientHandler implements Runnable {
        private final Socket clientSocket;
        private final String clientAddress;
        private BufferedReader in;
        private PrintWriter out;

        public ClientHandler(Socket clientSocket, String clientAddress) {
            this.clientSocket = clientSocket;
            this.clientAddress = clientAddress;
        }

        @Override
        public void run() {
            try {
                in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
                out = new PrintWriter(clientSocket.getOutputStream(), true);

                String clientMessage;
                while ((clientMessage = in.readLine()) != null) {
                    logServer.appendLog("Received raw data from ESP32: " + clientMessage);

                    // Parse JSON-data
                    try {
                        JSONObject json = new JSONObject(clientMessage);
                        String srcAddr = json.optString("src_addr", "UNKNOWN");
                        String payload = json.optString("payload", "UNKNOWN");

                        logServer.appendLog("Parsed JSON: src_addr=" + srcAddr + ", payload=" + payload);

                        processCommand(payload);

                        String serverResponse = "{\"response\":\"Command received\",\"status\":\"OK\"}";
                        out.println(serverResponse);
                        logServer.appendLog("Sent to ESP32: " + serverResponse);
                    } catch (JSONException e) {
                        logServer.appendLog("Error parsing JSON: " + e.getMessage());
                    }
                }
            } catch (IOException e) {
                logServer.appendLog("Connection error with client: " + clientAddress + ". Error: " + e.getMessage());
            } finally {
                try {
                    if (!clientSocket.isClosed()) {
                        clientSocket.close();
                        logServer.appendLog("Client disconnected: " + clientAddress);
                    }
                } catch (IOException e) {
                    logServer.appendLog("Error closing client socket: " + e.getMessage());
                } finally {
                    activeClients.remove(clientAddress);
                    clientSockets.remove(clientAddress);
                }
            }
        }
    }

    public void processCommand(String command) {
        logServer.appendLog("Command received in main server: " + command);

        if (command.equals("ADD_FIRE")) {
            if (gameModel.addFireOrSmokeRandomly(CellStatus.FIRE)) {
                logServer.appendLog("Fire added to the map.");
            }
        } else if (command.equals("ADD_SMOKE")) {
            if (gameModel.addFireOrSmokeRandomly(CellStatus.SMOKE)) {
                logServer.appendLog("Smoke added to the map.");
            }
        } else if (command.startsWith("CREATE_FIREMAN")) {
            String[] parts = command.split(" ");
            if (parts.length > 1) {
                String nodeId = parts[1];
                gameModel.moveObject("Firefighter", nodeId.hashCode(), 0, 0);
                logServer.appendLog("Firefighter created for node: " + nodeId);

                new Thread(() -> {
                    boolean moving = true;
                    while (moving) {
                        if (!gameModel.hasFireOrSmoke()) {
                            moving = false;
                            logServer.appendLog("Firefighter extinguished all fires/smoke.");
                            continue;
                        }

                        gameModel.moveFirefighterStepByStep(nodeId.hashCode());
                        if (visualisationServer != null) {
                            visualisationServer.updateVisualisation();
                        }

                        GameObject firefighter = gameModel.getObjects().stream()
                                .filter(obj -> obj.getType().equals("Firefighter") && obj.getId() == nodeId.hashCode())
                                .findFirst()
                                .orElse(null);

                        if (firefighter != null) {
                            int currentRow = firefighter.getRow();
                            int currentCol = firefighter.getCol();
                            logServer.appendLog("Firefighter moved to cell (" + currentRow + ", " + currentCol + ").");

                            if (gameModel.getCells()[currentRow][currentCol].getStatus() == CellStatus.EMPTY) {
                                logServer.appendLog("Firefighter extinguished fire/smoke at cell (" + currentRow + ", " + currentCol + ").");
                            }
                        }

                        try {
                            Thread.sleep(500);
                        } catch (InterruptedException e) {
                            logServer.appendLog("Error: " + e.getMessage());
                        }
                    }
                }).start();
            } else {
                logServer.appendLog("Invalid CREATE_FIREMAN command format.");
            }
        } else {
            logServer.appendLog("Unknown command received: " + command);
        }

        if (visualisationServer != null) {
            visualisationServer.updateVisualisation();
        }
    }

    public void monitorClients() {
        while (true) {
            try {
                Set<String> disconnectedClients = ConcurrentHashMap.newKeySet();
                for (String client : activeClients) {
                    if (!isClientConnected(client)) {
                        logServer.appendLog("Client disconnected: " + client);
                        disconnectedClients.add(client);
                    }
                }

                for (String client : disconnectedClients) {
                    activeClients.remove(client);
                    closeSocket(clientSockets.remove(client));
                }

                Thread.sleep(5000); // Kontrollera var 5:e sekund
            } catch (InterruptedException e) {
                logServer.appendLog("Error in client monitor: " + e.getMessage());
            }
        }
    }

    private boolean isClientConnected(String clientAddress) {
        Socket socket = clientSockets.get(clientAddress);
        if (socket == null) {
            return false;
        }
        try {
            socket.sendUrgentData(0xFF); // Skicka testbyte
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    private void closeSocket(Socket socket) {
        if (socket != null) {
            try {
                if (!socket.isClosed()) {
                    socket.close();
                    logServer.appendLog("Socket closed for client: " + socket.getInetAddress());
                }
            } catch (IOException e) {
                logServer.appendLog("Error closing socket: " + e.getMessage());
            }
        }
    }

    public static void main(String[] args) {
        GameModel gameModel = new GameModel(6, 8);
        LogServerGUI logServer = new LogServerGUI();

        MainServer mainServer = new MainServer(gameModel, logServer, null);

        VisualisationServer visualisationServer = new VisualisationServer(
                "src/resources/img5.jpg",
                gameModel,
                command -> new Thread(() -> mainServer.processCommand(command)).start()
        );

        mainServer.setVisualisationServer(visualisationServer);
        mainServer.startServer(8070);
    }
}
