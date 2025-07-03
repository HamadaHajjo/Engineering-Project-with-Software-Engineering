package Model;

import java.util.ArrayList;
import java.util.List;

public class GameModel {
    private final int rows;
    private final int cols;
    private final Cell[][] cells;
    private final List<GameObject> objects;

    public GameModel(int rows, int cols) {
        this.rows = rows;
        this.cols = cols;
        this.cells = new Cell[rows][cols];
        this.objects = new ArrayList<>();

        // Initiera cellerna som tomma
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                cells[r][c] = new Cell(r, c, CellStatus.EMPTY);
            }
        }
    }
    public boolean addFireOrSmokeRandomly(CellStatus status) {
        for (int i = 0; i < rows * cols; i++) { // Begränsa försök till antalet celler
            int row = (int) (Math.random() * rows);
            int col = (int) (Math.random() * cols);

            Cell cell = cells[row][col];
            if (cell.getStatus() == CellStatus.EMPTY) { // Kontrollera om cellen är tom
                cell.setStatus(status); // Uppdatera cellstatus till eld/rök
                System.out.println(status + " placerades på (" + row + ", " + col + ").");

                return true; // Eld/rök placerades
            }
        }

        System.out.println("Ingen tom cell hittades för att lägga till " + status + ".");
        return false; // Ingen eld/rök kunde placeras
    }



    public void moveFirefighterStepByStep(int id) {
        GameObject firefighter = null;

        // Hitta brandmannen
        for (GameObject obj : objects) {
            if (obj.getType().equals("Firefighter") && obj.getId() == id) {
                firefighter = obj;
                break;
            }
        }

        if (firefighter == null) return; // Ingen brandman hittad

        // Hitta närmaste cell med eld eller rök
        int targetRow = -1, targetCol = -1;
        int shortestDistance = Integer.MAX_VALUE;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (cells[r][c].getStatus() == CellStatus.FIRE || cells[r][c].getStatus() == CellStatus.SMOKE) {
                    int distance = Math.abs(firefighter.getRow() - r) + Math.abs(firefighter.getCol() - c);
                    if (distance < shortestDistance) {
                        shortestDistance = distance;
                        targetRow = r;
                        targetCol = c;
                    }
                }
            }
        }

        if (targetRow == -1 || targetCol == -1) return; // Ingen eld eller rök hittad

        // Flytta ett steg i taget
        int currentRow = firefighter.getRow();
        int currentCol = firefighter.getCol();

        if (currentRow < targetRow) currentRow++;
        else if (currentRow > targetRow) currentRow--;

        if (currentCol < targetCol) currentCol++;
        else if (currentCol > targetCol) currentCol--;

        firefighter.setPosition(currentRow, currentCol);

        // Logga varje steg
        System.out.println("Brandmannen flyttad till (" + currentRow + ", " + currentCol + ").");

        // Släck eld eller rök om den når målet
        if (currentRow == targetRow && currentCol == targetCol) {
            cells[targetRow][targetCol].setStatus(CellStatus.EMPTY);
            System.out.println("Brandmannen släcker eld/rök på cell (" + targetRow + ", " + targetCol + ").");
        }
    }




    public boolean hasFireOrSmoke() {
        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                if (cells[r][c].getStatus() == CellStatus.FIRE || cells[r][c].getStatus() == CellStatus.SMOKE) {
                    return true;
                }
            }
        }
        return false;
    }



    public void moveObject(String type, int id, int row, int col) {
        for (GameObject obj : objects) {
            if (obj.getType().equals(type) && obj.getId() == id) {
                System.out.println("Flyttar brandmannen till (" + row + ", " + col + ").");

                if (cells[row][col].getStatus() == CellStatus.FIRE || cells[row][col].getStatus() == CellStatus.SMOKE) {
                    System.out.println("Brandmannen släcker eld/rök på cellen (" + row + ", " + col + ").");
                    cells[row][col].setStatus(CellStatus.EMPTY);
                }

                obj.setPosition(row, col);
                return;
            }
        }

        System.out.println("Skapar en ny brandman på (" + row + ", " + col + ").");
        objects.add(new GameObject(type, id, row, col));
    }

    public Cell[][] getCells() {
        return cells;
    }

    public List<GameObject> getObjects() {
        return objects;
    }
}
