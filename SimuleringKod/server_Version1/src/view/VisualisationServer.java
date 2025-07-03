package view;

import javax.swing.*;
import java.awt.*;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;
import Model.Cell;
import Model.GameModel;
import Model.GameObject;


import javax.swing.*;
import java.awt.*;
import Model.Cell;
import Model.GameModel;
import Model.GameObject;
import java.util.function.Consumer; // För att skicka kommandon till MainServer

public class VisualisationServer extends JFrame {
    private final GameModel gameModel;
    private final Image mapImage;
    private final Image firefighterImage;
    private final Image fireImage;
    private final Image smokeImage;

    private final int gridRows = 6; // Antal rader i grid
    private final int gridCols = 8; // Antal kolumner i grid

    private final Consumer<String> commandSender; // För att skicka kommandon till MainServer

    public VisualisationServer(String imagePath, GameModel gameModel, Consumer<String> commandSender) {
        this.gameModel = gameModel;
        this.commandSender = commandSender;

        // Ladda bilder
        mapImage = new ImageIcon(imagePath).getImage();
        firefighterImage = new ImageIcon(getClass().getResource("/resources/brandman.png")).getImage();
        fireImage = new ImageIcon(getClass().getResource("/resources/fire.png")).getImage();
        smokeImage = new ImageIcon(getClass().getResource("/resources/smoke.png")).getImage();

        // GUI-Komponenter
        JButton addFireButton = new JButton("Lägg till eld");
        JButton addSmokeButton = new JButton("Lägg till rök");

        // Skicka kommandon till MainServer när knappar trycks
        addFireButton.addActionListener(e -> commandSender.accept("ADD_FIRE"));
        addSmokeButton.addActionListener(e -> commandSender.accept("ADD_SMOKE"));

        JPanel buttonPanel = new JPanel();
        buttonPanel.add(addFireButton);
        buttonPanel.add(addSmokeButton);

        setLayout(new BorderLayout());
        add(new VisualisationPanel(), BorderLayout.CENTER); // Karta och objekt
        add(buttonPanel, BorderLayout.SOUTH); // Knappar längst ner

        setTitle("Visualisering - Brand Simulering");
        setSize(1000, 600);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        setVisible(true);
    }

    public void updateVisualisation() {
        SwingUtilities.invokeLater(this::repaint); // Uppdatera GUI
    }

    private class VisualisationPanel extends JPanel {
        @Override
        protected void paintComponent(Graphics g) {
            super.paintComponent(g);

            // Rita bakgrund
            g.drawImage(mapImage, 0, 0, getWidth(), getHeight(), this);

            // Dynamisk cellstorlek
            int cellWidth = getWidth() / gridCols;
            int cellHeight = getHeight() / gridRows;

            // Rita cellstatus (eld, rök)
            Cell[][] cells = gameModel.getCells();
            for (int r = 0; r < cells.length; r++) {
                for (int c = 0; c < cells[r].length; c++) {
                    Cell cell = cells[r][c];
                    int x = c * cellWidth;
                    int y = r * cellHeight;

                    switch (cell.getStatus()) {
                        case FIRE -> g.drawImage(fireImage, x, y, cellWidth, cellHeight, this);
                        case SMOKE -> g.drawImage(smokeImage, x, y, cellWidth, cellHeight, this);
                    }
                }
            }

            // Rita objekt (t.ex. brandmannen)
            for (GameObject obj : gameModel.getObjects()) {
                int x = obj.getCol() * cellWidth;
                int y = obj.getRow() * cellHeight;

                if ("Firefighter".equals(obj.getType())) {
                    g.drawImage(firefighterImage, x, y, cellWidth, cellHeight, this);
                }
            }
        }
    }
}
