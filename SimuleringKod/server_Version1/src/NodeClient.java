import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;

public class NodeClient {
    private static final String SERVER_ADDRESS = "localhost";
    private static final int SERVER_PORT = 1234;
    private final String nodeId;

    public NodeClient(String nodeId) {
        this.nodeId = nodeId;
    }

    public void start() {
        new Thread(() -> {
            try (Socket socket = new Socket(SERVER_ADDRESS, SERVER_PORT);
                 PrintWriter out = new PrintWriter(socket.getOutputStream(), true)) {

                // Skicka begäran om att skapa brandman
                out.println("CREATE_FIREMAN " + nodeId);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }).start();
    }

    public static void main(String[] args) {
        new NodeClient("Node A").start();
    }
}
