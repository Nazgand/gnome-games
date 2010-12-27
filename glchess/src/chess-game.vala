public enum Color
{
    WHITE,
    BLACK
}

public class ChessPlayer : GLib.Object
{
    public Color color;
    public signal void start_turn ();
    public signal bool do_move (string move, bool apply);
    public signal bool do_move_with_coords (int r0, int f0, int r1, int f1, bool apply);
    public signal bool do_resign ();
    public signal bool do_claim_draw ();

    public ChessPlayer (Color color)
    {
        this.color = color;
    }

    public bool move (string move, bool apply = true)
    {
        return do_move (move, apply);
    }

    public bool move_with_coords (int r0, int f0, int r1, int f1, bool apply = true)
    {
        return do_move_with_coords (r0, f0, r1, f1, apply);
    }

    public bool resign ()
    {
        return do_resign ();
    }
    
    public bool claim_draw ()
    {
        return do_claim_draw ();
    }
}

public enum PieceType
{
    PAWN,
    ROOK,
    KNIGHT,
    BISHOP,
    QUEEN,
    KING
}

public class ChessPiece
{
    public ChessPlayer player;
    public PieceType type;

    public signal void moved ();
    public signal void promoted ();
    public signal void died ();

    public ChessPiece (ChessPlayer player, PieceType type)
    {
        this.player = player;
        this.type = type;
    }
}

public class ChessMove
{
    public int number;
    public ChessPiece piece;
    public ChessPiece? moved_rook;
    public ChessPiece? victim;
    public int r0;
    public int f0;
    public int r1;
    public int f1;
    public string lan = "";
    public string san = "";
    public string human = "";
    
    public string fan
    {
        get
        {
            // FIXME: Translate the san move
            //♙♘♗♖♕♔
            //♟♞♝♜♛♚
            return san;
        }
    }

    public ChessMove copy ()
    {
        var move = new ChessMove ();
        move.number = number;
        move.piece = piece;
        move.moved_rook = moved_rook;
        move.victim = victim;
        move.r0 = r0;
        move.f0 = f0;
        move.r1 = r1;
        move.f1 = f1;
        move.lan = lan;        
        move.san = san;
        move.human = human;
        return move;
    }
}

public class ChessState
{
    public int number = 0;
    public ChessPlayer players[2];
    public ChessPlayer current_player;
    public ChessPlayer opponent;
    public bool white_can_castle_kingside;
    public bool white_can_castle_queenside;
    public bool black_can_castle_kingside;
    public bool black_can_castle_queenside;

    public ChessPiece[] board;
    public ChessMove? last_move = null;

    /* Bitmap of all the pieces */
    private int64 piece_masks[2];

    public ChessState (string? state = null)
    {
        players[Color.WHITE] = new ChessPlayer (Color.WHITE);
        players[Color.BLACK] = new ChessPlayer (Color.BLACK);
        board = new ChessPiece[64];
        for (int i = 0; i < 64; i++)
            board[i] = null;

        if (state == null)
            return;

        string[] fields = state.split (" ");
        //if (fields.length != 6)
        //    throw new Error ("Invalid FEN string");

        /* Field 1: Piece placement */
        string[] ranks = fields[0].split ("/");
        //if (ranks.length != 8)
        //    throw new Error ("Invalid piece placement");
        for (int rank = 0; rank < 8; rank++)
        {
            var rank_string = ranks[7 - rank];
            for (int file = 0, offset = 0; file < 8 && offset < rank_string.length; offset++)
            {
                var c = rank_string[offset];
                if (c >= '1' && c <= '8')
                {
                    file += c - '0';
                    continue;
                }

                PieceType type;
                var color = c.isupper () ? Color.WHITE : Color.BLACK;
                if (!decode_piece_type (c.toupper (), out type))
                    ;//throw new Error ("");

                int index = get_index (rank, file);
                ChessPiece piece = new ChessPiece (players[color], type);
                board[index] = piece;
                int64 mask = BitBoard.set_location_masks[index];
                piece_masks[color] |= mask;
                file++;
            }
        }

        /* Field 2: Active color */
        if (fields[1] == "w")
        {
            current_player = players[Color.WHITE];
            opponent = players[Color.BLACK];
        }
        else if (fields[1] == "b")
        {
            current_player = players[Color.BLACK];
            opponent = players[Color.WHITE];
        }
        //else
        //    throw new Error ("Unknown active color: %s", fields[1]);

        /* Field 3: Castling availability */
        for (int i = 0; i < fields[2].length; i++)
        {
            var c = fields[2][i];
            if (c == '-')
                ; /* No availability */
            else if (c == 'K')
                white_can_castle_kingside = true;
            else if (c == 'q')
                white_can_castle_queenside = true;
            else if (c == 'k')
                black_can_castle_kingside = true;
            else if (c == 'q')
                black_can_castle_queenside = true;
            //else
            //    throw new Error ("");
        }

        /* Field 4: En passant target square */
        // FIXME
        
        /* Field 5: Halfmove count */
        // FIXME

        /* Field 6: Fullmove number */
        // FIXME
    }

    public ChessState copy ()
    {
        ChessState state = new ChessState ();

        state.number = number;
        state.players[Color.WHITE] = players[Color.WHITE];
        state.players[Color.BLACK] = players[Color.BLACK];
        state.current_player = current_player;
        state.opponent = opponent;
        state.white_can_castle_kingside = white_can_castle_kingside;
        state.white_can_castle_queenside = white_can_castle_queenside;
        state.black_can_castle_kingside = black_can_castle_kingside;
        state.black_can_castle_queenside = black_can_castle_queenside;
        if (last_move != null)
            state.last_move = last_move.copy();
        for (int i = 0; i < 64; i++)
            state.board[i] = board[i];
        state.piece_masks[Color.WHITE] = piece_masks[Color.WHITE];
        state.piece_masks[Color.BLACK] = piece_masks[Color.BLACK];

        return state;
    }

    public int get_index (int rank, int file)
    {
        return rank * 8 + file;
    }
    
    public int get_rank (int index)
    {
        return index / 8;
    }

    public int get_file (int index)
    {
        return index % 8;
    }

    public bool move (string move, bool apply = true)
    {
        int r0, f0, r1, f1;
        PieceType promotion_type;
        if (!decode_move (move, out r0, out f0, out r1, out f1, out promotion_type))
            return false;
        return move_with_coords (r0, f0, r1, f1, promotion_type, apply);
    }

    public bool move_with_coords (int r0, int f0, int r1, int f1, PieceType promotion_type = PieceType.QUEEN, bool apply = true, bool test_check = true)
    {
        // FIXME: Make this use indexes to be faster
        int start = get_index (r0, f0);
        int end = get_index (r1, f1);

        var color = current_player.color;
        var opponent_color = opponent.color;

        /* Must be moving own piece */
        ChessPiece? piece = board[start];
        if (piece == null || piece.player != current_player)
            return false;

        /* Check valid move */
        int64 end_mask = BitBoard.set_location_masks[end];
        int64 move_mask = BitBoard.move_masks[color * 64*6 + piece.type * 64 + start];
        if ((end_mask & move_mask) == 0)
            return false;

        /* Check no pieces in the way */
        int64 over_mask = BitBoard.over_masks[start * 64 + end];
        if ((over_mask & (piece_masks[Color.WHITE] | piece_masks[Color.BLACK])) != 0)
            return false;

        /* Can't take own pieces */
        ChessPiece? victim = board[end];
        if (victim != null && victim.player == current_player)
            return false;
            
        /* Check special moves */
        int rook_start = -1, rook_end = -1;
        switch (piece.type)
        {
        case PieceType.PAWN:
            /* If moving diagonally there must be a victim */
            if (f0 != f1)
            {
                if (victim == null)
                    return false;
                // FIXME: Check en passant
            }
            else
            {
                /* If moving forward can't take enemy */
                if (victim != null)
                    return false;
            }
            break;
        case PieceType.KING:
            /* If moving more than one square must be castling */
            if ((f0 - f1).abs () > 1)
            {
                /* File the rook is on */
                rook_start = get_index (r0, f1 > f0 ? 7 : 0);
                rook_end = get_index (r0, f1 > f0 ? f1 - 1 : f1 + 1);

                ChessPiece? rook = board[rook_start];
                if (rook == null)
                    return false;

                /* Check rook can move */
                int64 rook_over_mask = BitBoard.over_masks[rook_start * 64 + rook_end];
                if ((rook_over_mask & (piece_masks[Color.WHITE] | piece_masks[Color.BLACK])) != 0)
                    return false;

                // FIXME: Can only castle if:
                // King hasn't moved
                // Rook hasn't moved
                // Can't be in check while king moved (check if rook can be taken?)
            }
            break;
        default:
            break;
        }

        if (!apply && !test_check)
            return true;

        var old_white_mask = piece_masks[Color.WHITE];
        var old_black_mask = piece_masks[Color.BLACK];
        var old_current_player = current_player;
        var old_opponent = opponent;

        /* Update board */
        board[start] = null;
        if (piece.type == PieceType.PAWN && (r1 == 0 || r1 == 7))
            board[end] = new ChessPiece (old_current_player, promotion_type);
        else
            board[end] = piece;
        piece_masks[Color.WHITE] &= BitBoard.clear_location_masks[start];
        piece_masks[Color.BLACK] &= BitBoard.clear_location_masks[start];
        piece_masks[color] |= end_mask;
        piece_masks[opponent_color] &= BitBoard.clear_location_masks[end];
        if (rook_start >= 0)
        {
            var rook = board[rook_start];
            board[rook_start] = null;
            board[rook_end] = rook;
            piece_masks[color] &= BitBoard.clear_location_masks[rook_start];
            piece_masks[color] |= BitBoard.set_location_masks[rook_end];
        }
        current_player = old_opponent;
        opponent = old_current_player;

        /* Test if this move would leave that player in check */
        bool result = true;
        if (test_check)
        {
            for (int king_index = 0; king_index < 64; king_index++)
            {
                var p = board[king_index];
                if (p != null && p.player == opponent && p.type == PieceType.KING)
                {
                    /* See if any enemy pieces can take the king */
                    for (int i = 0; i < 64; i++)
                    {
                        // FIXME: Should check all promotion types...
                        if (move_with_coords (get_rank (i), get_file (i), get_rank (king_index), get_file (king_index), PieceType.QUEEN, false, false))
                        {
                            result = false;
                            break;
                        }
                    }
                }
            }
        }

        /* Undo move */
        if (!apply || !result)
        {
            current_player = old_current_player;
            opponent = old_opponent;
            board[start] = piece;
            board[end] = victim;
            if (rook_start >= 0)
            {
                var rook = board[rook_end];
                board[rook_start] = rook;
                board[rook_end] = null;
            }
            piece_masks[Color.WHITE] = old_white_mask;
            piece_masks[Color.BLACK] = old_black_mask;
        }

        if (!apply)
            return result;

        last_move = new ChessMove ();
        last_move.number = number;
        last_move.piece = piece;
        last_move.victim = victim;
        if (rook_end >= 0)
            last_move.moved_rook = board[rook_end];
        last_move.r0 = r0;
        last_move.f0 = f0;
        last_move.r1 = r1;
        last_move.f1 = f1;
        // FIXME: Promotion
        last_move.lan = "%c%d%c%d".printf ('a' + f0, r0 + 1, 'a' + f1, r1 + 1);
        // FIXME: Generate SAN move
        last_move.san = last_move.lan;

        return true;
    }

    private bool decode_piece_type (unichar c, out PieceType type)
    {
        switch (c)
        {
        case 'P':
            type = PieceType.PAWN;
            return true;
        case 'R':
            type = PieceType.ROOK;
            return true;
        case 'N':
            type = PieceType.KNIGHT;
            return true;
        case 'B':
            type = PieceType.BISHOP;
            return true;
        case 'Q':
            type = PieceType.QUEEN;
            return true;
        case 'K':
            type = PieceType.KING;
            return true;
        default:
            return false;
        }
    }

    private bool decode_move (string move, out int r0, out int f0, out int r1, out int f1, out PieceType promotion_type)
    {
        int i = 0;

        promotion_type = PieceType.QUEEN;
        if (move.has_prefix ("O-O-O"))
        {
            if (current_player.color == Color.WHITE)
                r0 = r1 = 0;
            else
                r0 = r1 = 7;
            f0 = 4;
            f1 = 2;
            i += (int) "O-O-O".length;
        }
        else if (move.has_prefix ("O-O"))
        {
            if (current_player.color == Color.WHITE)
                r0 = r1 = 0;
            else
                r0 = r1 = 7;
            f0 = 4;
            f1 = 6;
            i += (int) "O-O".length;
        }
        else
        {
            PieceType type = PieceType.PAWN;
            if (decode_piece_type (move[i], out type))
                i++;

            r0 = f0 = r1 = f1 = -1;
            if (move[i] >= 'a' && move[i] <= 'h')
            {
                f1 = (int) (move[i] - 'a');
                i++;
            }
            if (move[i] >= '1' && move[i] <= '8')
            {
                r1 = (int) (move[i] - '1');
                i++;
            }
            if (move[i] == 'x')
                i++;
            if (move[i] >= 'a' && move[i] <= 'h')
            {
                f0 = f1;
                f1 = (int) (move[i] - 'a');
                i++;
            }
            if (move[i] >= '1' && move[i] <= '8')
            {
                r0 = r1;
                r1 = (int) (move[i] - '1');
                i++;
            }
            if (move[i] == '=')
                i++;
            if (decode_piece_type (move[i], out promotion_type))
                i++;

            /* Don't have a destination to move to */
            if (r1 < 0 || f1 < 0)
            {
                GLib.debug ("Move %s missing destination", move);
                return false;
            }

            /* Find source piece */
            if (r0 < 0 || f0 < 0)
            {
                int match_rank = -1, match_file = -1;

                for (int file = 0; file < 8; file++)
                {
                    if (f0 >= 0 && file != f0)
                        continue;

                    for (int rank = 0; rank < 8; rank++)
                    {
                        if (r0 >= 0 && rank != r0)
                            continue;

                        /* Only check this players pieces of the correct type */
                        var piece = board[get_index (rank, file)];
                        if (piece == null || piece.type != type || piece.player != current_player)
                            continue;

                        /* See if can move here */
                        // FIXME: Should check all promotion types...
                        if (!this.move_with_coords (rank, file, r1, f1, PieceType.QUEEN, false))
                            continue;

                        /* Duplicate match */
                        if (match_rank >= 0)
                        {
                            GLib.debug ("Move %s is ambiguous", move);
                            return false;
                        }

                        match_rank = rank;
                        match_file = file;
                    }
                }

                if (match_rank < 0)
                {
                    GLib.debug ("Move %s has no matches", move);
                    return false;
                }

                r0 = match_rank;
                f0 = match_file;
            }
        }

        if (move[i] == '+')
            i++;
        else if (move[i] == '#')
            i++;

        if (move[i] != '\0')
        {
            GLib.debug ("Move %s has unexpected characters", move);
            return false;
        }

        return true;
    }
}

public enum ChessResult
{
    IN_PROGRESS,
    WHITE_WON,
    BLACK_WON,
    DRAW
}

public enum ChessRule
{
    CHECKMATE,
    STALEMATE,
    FIFTY_MOVES,
    TIMEOUT,
    THREE_FOLD_REPETITION,
    INSUFFICIENT_MATERIAL,
    RESIGN,
    ABANDONMENT,
    DEATH
}

public class ChessGame
{
    public bool is_started;
    public ChessResult result;
    public ChessRule rule;
    public GLib.List<ChessState> move_stack;

    public signal void started ();
    public signal void turn_started (ChessPlayer player);
    public signal void moved (ChessMove move);
    public signal void ended ();

    public ChessPlayer white
    {
        get { return move_stack.data.players[Color.WHITE]; }
    }
    public ChessPlayer black
    {
        get { return move_stack.data.players[Color.BLACK]; }
    }
    public ChessPlayer current_player
    {
        get { return move_stack.data.current_player; }
    }

    public ChessGame (string state = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1")
    {
        is_started = false;
        move_stack.prepend (new ChessState (state));
        result = ChessResult.IN_PROGRESS;

        white.do_move.connect (move_cb);
        white.do_move_with_coords.connect (move_with_coords_cb);
        white.do_resign.connect (resign_cb);
        white.do_claim_draw.connect (claim_draw_cb);
        black.do_move.connect (move_cb);
        black.do_move_with_coords.connect (move_with_coords_cb);
        black.do_resign.connect (resign_cb);
        black.do_claim_draw.connect (claim_draw_cb);
    }

    private bool move_cb (ChessPlayer player, string move, bool apply)
    {
        return do_move (player, move, -1, -1, -1, -1, apply);
    }

    private bool move_with_coords_cb (ChessPlayer player, int r0, int f0, int r1, int f1, bool apply)
    {
        return do_move (player, null, r0, f0, r1, f1, apply);
    }

    private bool do_move (ChessPlayer player, string? move, int r0, int f0, int r1, int f1, bool apply)
    {
        if (!is_started)
            return false;
        if (player != current_player)
            return false;

        var state = move_stack.data.copy ();
        state.number++;
        if (move != null)
        {
            if (!state.move (move, apply))
                return false;
        }
        else
        {
            if (!state.move_with_coords (r0, f0, r1, f1, PieceType.QUEEN, apply))
                return false;
        }

        if (!apply)
            return true;

        move_stack.prepend (state);
        if (state.last_move.victim != null)
            state.last_move.victim.died ();
        state.last_move.piece.moved ();
        if (state.last_move.moved_rook != null)
            state.last_move.moved_rook.moved ();
        moved (state.last_move);

        current_player.start_turn ();
        turn_started (current_player);

        return true;
    }

    private bool resign_cb (ChessPlayer player)
    {
        if (!is_started)
            return false;

        if (player.color == Color.WHITE)
            stop (ChessResult.BLACK_WON, ChessRule.RESIGN);
        else
            stop (ChessResult.WHITE_WON, ChessRule.RESIGN);

        return true;
    }

    private bool claim_draw_cb (ChessPlayer player)
    {
        if (!is_started)
            return false;

        // FIXME: Check if can

        stop (ChessResult.DRAW, ChessRule.FIFTY_MOVES);

        return true;
    }

    public void start ()
    {
        if (is_started)
            return;
        is_started = true;

        reset ();

        started ();
        current_player.start_turn ();
        turn_started (current_player);
    }
    
    public void abandon ()
    {
        if (!is_started)
            return;
        stop (ChessResult.DRAW, ChessRule.ABANDONMENT);
    }
    
    public ChessPiece? get_piece (int rank, int file, int move_number = -1)
    {
        if (move_number < 0)
            move_number += (int) move_stack.length ();

        var state = move_stack.nth_data (move_stack.length () - move_number - 1);

        return state.board[state.get_index (rank, file)];
    }

    public uint n_moves
    {
        get { return move_stack.length() - 1; }
    }

    private void reset ()
    {
        result = ChessResult.IN_PROGRESS;
        var state = move_stack.data;
        move_stack = null;
        move_stack.prepend (state);
    }

    private void stop (ChessResult result, ChessRule rule)
    {
        this.result = result;
        this.rule = rule;
        is_started = false;
        ended ();
    }
}
