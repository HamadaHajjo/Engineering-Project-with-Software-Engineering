package Model;

public class Cell {
    private final int row;
    private final int col;
    private CellStatus status;

    public Cell(int row, int col, CellStatus status) {
        this.row = row;
        this.col = col;
        this.status = status;
    }

    public int getRow() {
        return row;
    }

    public int getCol() {
        return col;
    }

    public CellStatus getStatus() {
        return status;
    }

    public void setStatus(CellStatus status) {
        this.status = status;
    }
}
