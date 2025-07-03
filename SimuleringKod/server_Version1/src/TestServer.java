import Model.CellStatus;
import Model.GameModel;

import java.io.PrintWriter;

public class TestServer {
    private final GameModel gameModel;

    public TestServer(GameModel gameModel) {
        this.gameModel = gameModel;
    }

    public void simulateFireEvent(PrintWriter out) {
        out.println("ADD_FIRE");
        System.out.println("Simulerade eld via ADD_FIRE.");
    }

    public void simulateSmokeEvent(PrintWriter out) {
        out.println("ADD_SMOKE");
        System.out.println("Simulerade rök via ADD_SMOKE.");
    }
}
