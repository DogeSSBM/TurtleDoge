#include "DogeLib/Includes.h"

typedef enum{E_ROR, E_ROL, E_FWD, E_SET, E_CLR, E_END, E_ERR, E_N}ExprType;
char *ExprTypeStr[E_N] = {"E_ROR", "E_ROL", "E_FWD", "E_SET", "E_CLR", "E_END", "E_ERR"};
typedef struct Expr{
    ExprType type;
    struct Expr *next;
}Expr;

Expr* exprConsume(Expr *expr)
{
    if(!expr)
        return NULL;
    Expr *next = expr->next;
    free(expr);
    return next;
}

Expr* exprAppend(Expr *head, Expr *tail)
{
    if(!head)
        return tail;
    Expr *cur = head;
    while(cur->next)
        cur = cur->next;
    cur->next = tail;
    return head;
}

ExprType exprTypeParse(char *str)
{
    for(ExprType type = 0; type < E_N; type++){
        if(strncmp(str, ExprTypeStr[type], 5) == 0)
            return type;
    }
    return E_ERR;
}

Expr* exprParse(char *str)
{
    assertExpr(str);
    Expr *head = NULL;
    Expr *cur = NULL;
    do{
        while(isspace(*str))
            str++;
        const st slen = strlen(str);
        cur = calloc(1, sizeof(Expr));
        if(slen == 0){
            cur->type = E_END;
        }else{
            assertExpr(slen >= 3);
            cur->type = exprTypeParse(str);
            assertExprMsg(cur->type != E_ERR, "%s", str);
            str += 5;
        }
        head = exprAppend(head, cur);
    }while(cur->type != E_END);
    return head;
}

typedef struct Turtle{
    uint delay;
    int nextUpdate;
    int scale;
    Length win;
    Length len;
    Coord origin;
    Coord pos;
    Direction dir;
    Offset off;
    bool **grid;
    Expr *expr;
}Turtle;

int gridScale(const Length winLen, const Length gridLen)
{
    return imin(winLen.x/gridLen.x, winLen.y/gridLen.y);
}

Offset gridOffset(const Length winLen, const Length gridLen, const int scale)
{
    return coordDivi(coordSub(winLen, coordMuli(gridLen, scale)), 2);
}

Coord winGridPos(const int scale, const Offset gridOff, const Coord winPos)
{
    return coordDivi(coordSub(winPos, gridOff), scale);
}

Coord gridWinPos(const int scale, const Offset gridOff, const Coord gridPos)
{
    return coordAdd(coordMuli(gridPos, scale), gridOff);
}

void turtleFree(Turtle t)
{
    if(!t.grid)
        return;
    for(int i = 0; i < t.len.x; i++)
        if(t.grid[i])
            free(t.grid[i]);
    free(t.grid);
    while((t.expr = exprConsume(t.expr)));
}

void turtleDraw(const Turtle t)
{
    assertExpr(t.grid);
    for(int y = 0; y < t.len.y; y++){
        for(int x = 0; x < t.len.x; x++){
            const Coord gpos = iC(x,y);
            const Coord wpos = gridWinPos(t.scale, t.off, gpos);
            const bool state = t.grid[x][y];
            setColor(state?WHITE:BLACK);
            fillSquareCoord(wpos, t.scale);
            setColor(state?BLACK:WHITE);
            fillBorderCoordSquare(wpos, t.scale, -1);

        }
    }
    const Coord wpos = gridWinPos(t.scale, t.off, t.pos);
    setColor(GREEN);
    fillSquareCoord(wpos, t.scale);
    assertExpr(coordInLen(t.pos, t.len));
    const bool state = t.grid[t.pos.x][t.pos.y];
    setColor(state?BLACK:WHITE);
    fillBorderCoordSquare(wpos, t.scale, -1);
    const Coord mid = coordAddi(wpos, t.scale/2);
    const Coord tri[3] = {
        coordShift(mid, t.dir, t.scale/2),
        coordShift(mid, dirROL(t.dir), t.scale/2),
        coordShift(mid, dirROR(t.dir), t.scale/2)
    };
    setColor(WHITE);
    fillPoly(tri, 3);
}

Turtle gridRescale(Turtle t)
{
    t.win = getWindowLen();
    t.scale = gridScale(t.win, t.len);
    t.off = gridOffset(t.win, t.len, t.scale);
    return t;
}

Expr* exprParseFile(const char *path)
{
    char *str = fileReadText(path);
    Expr *expr = exprParse(str);
    free(str);
    return expr;
}

Turtle turtleNew(const Length len, const Coord origin, Expr *expr)
{
    assertExpr(coordInLen(origin, len) && coordMin(len) > 0);
    Turtle t = {
        .delay = 1000,
        .win = getWindowLen(),
        .scale = gridScale(t.win, len),
        .len = len,
        .origin = origin,
        .pos = origin,
        .off = gridOffset(t.win, t.len, t.scale),
        .grid = calloc(len.x, sizeof(bool*)),
        .expr = expr
    };
    for(int i = 0; i < len.x; i++)
        t.grid[i] = calloc(len.y, sizeof(bool));
    return t;
}

Turtle turtleUpdate(Turtle t)
{
    printf("Turtle current expr: %s\n", ExprTypeStr[t.expr->type]);
    switch(t.expr->type){
        case E_ROR:
            t.dir = dirROR(t.dir);
            break;
        case E_ROL:
            t.dir = dirROL(t.dir);
            break;
        case E_FWD:
            t.pos = coordShift(t.pos, t.dir, 1);
            assertExpr(coordInLen(t.pos, t.len));
            break;
        case E_SET:
            assertExpr(coordInLen(t.pos, t.len));
            t.grid[t.pos.x][t.pos.y] = true;
            break;
        case E_CLR:
            assertExpr(coordInLen(t.pos, t.len));
            t.grid[t.pos.x][t.pos.y] = false;
            break;
        case E_END:
            break;
        case E_ERR:
        default:
            panic("Uh oh");
            break;
    }
    t.expr = exprConsume(t.expr);
    return t;
}

int main(int argc, char **argv)
{
    assertExpr(argc == 2);
    init();
    Turtle turtle = turtleNew(iC(9,9), iC(3, 3), exprParseFile(argv[1]));
    while(1){
        const int t = frameStart();

        if(keyState(SDL_SCANCODE_ESCAPE)){
            turtleFree(turtle);
            exit(EXIT_SUCCESS);
        }
        turtle = gridRescale(turtle);
        if(t > turtle.nextUpdate){
            turtle.nextUpdate = t + turtle.delay;
            turtle = turtleUpdate(turtle);
        }

        turtleDraw(turtle);

        frameEnd(t);
    }
    return 0;
}
