import javax.swing.*;
import java.awt.*;
public class LogServerGUI {
    private final JFrame frame;
    private final JTextArea logArea;

    public LogServerGUI() {
        frame = new JFrame("Log Server");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.setSize(500, 300);

        logArea = new JTextArea();
        logArea.setEditable(false);
        JScrollPane scrollPane = new JScrollPane(logArea);

        frame.setLayout(new BorderLayout());
        frame.add(scrollPane, BorderLayout.CENTER);

        frame.setVisible(true);
    }

    public void appendLog(String message) {
        SwingUtilities.invokeLater(() -> {
            logArea.append(message + "\n");
            logArea.setCaretPosition(logArea.getDocument().getLength()); // Scrolla automatiskt
        });
    }
}
