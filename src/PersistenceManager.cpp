#include "PersistenceManager.h"
#include "movegen.h"

#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <algorithm>
#include <cctype>
#include <sstream>

// ─────────────────────────────────────────────────────────────────────────────
//  Construction
// ─────────────────────────────────────────────────────────────────────────────

PersistenceManager::PersistenceManager(ChessController* ctrl, QObject* parent)
    : QObject(parent), m_ctrl(ctrl)
{
    // Wire controller signals → our slots
    connect(m_ctrl, &ChessController::moveApplied,
            this,   &PersistenceManager::onMoveApplied);
    connect(m_ctrl, &ChessController::undoPerformed,
            this,   &PersistenceManager::onUndoPerformed);
    connect(m_ctrl, &ChessController::boardChanged,
            this,   [this]{ m_dirty = true; });

    // Autosave timer
    connect(&m_autosaveTimer, &QTimer::timeout,
            this, &PersistenceManager::doAutosave);
    m_autosaveTimer.setInterval(m_autosaveInterval * 1000);
    m_autosaveTimer.start();

    // Load recent saves list from QSettings
    QSettings s("ChessQt", "ChessGUI");
    m_recentSaves = s.value("recentSaves").toStringList();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Slots — track move log
// ─────────────────────────────────────────────────────────────────────────────

void PersistenceManager::onMoveApplied(Move m, bool, bool, bool, bool, bool)
{
    // DSA: Vector — append to move log in O(1) amortised
    m_moveLog.push_back(m.toString());
    m_dirty = true;
}

void PersistenceManager::onUndoPerformed()
{
    // DSA: Vector — pop_back to undo last move(s)
    // In HvAI mode controller undoes 2 half-moves; we mirror that
    if (!m_moveLog.empty()) m_moveLog.pop_back();
    if (!m_moveLog.empty() &&
        (m_ctrl->config().mode == PlayerMode::HumanVsAI ||
         m_ctrl->config().mode == PlayerMode::AIVsHuman))
        m_moveLog.pop_back();
    m_dirty = true;
}

void PersistenceManager::onNewGame()
{
    clearMoveLog();
    m_dirty = false;
}

void PersistenceManager::clearMoveLog()
{
    m_moveLog.clear();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Autosave
// ─────────────────────────────────────────────────────────────────────────────

void PersistenceManager::setAutosaveEnabled(bool enabled)
{
    m_autosaveEnabled = enabled;
    if (enabled)  m_autosaveTimer.start();
    else          m_autosaveTimer.stop();
}

void PersistenceManager::setAutosaveIntervalSeconds(int secs)
{
    m_autosaveInterval = secs;
    m_autosaveTimer.setInterval(secs * 1000);
}

QString PersistenceManager::autosavePath() const
{
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/autosave.chsave";
}

void PersistenceManager::doAutosave()
{
    if (!m_autosaveEnabled || !m_dirty || m_moveLog.empty()) return;
    QString path = autosavePath();
    if (saveGame(path, "Autosave")) {
        m_dirty = false;
        emit autosaved(path);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Save Game
//  DSA: File I/O structures — custom key:value text format
// ─────────────────────────────────────────────────────────────────────────────

SavedGame PersistenceManager::buildSaveData(const QString& saveName) const
{
    SavedGame sg;
    sg.saveName   = saveName.isEmpty() ? "Unnamed" : saveName;
    sg.dateTime   = QDateTime::currentDateTime().toString(Qt::ISODate);
    // (removed)
    sg.playerMode = modeString();
    sg.difficulty = diffString();
    sg.humanColor = (m_ctrl->config().humanColor == WHITE) ? "White" : "Black";
    sg.moves      = m_moveLog;     // DSA: Vector copy
    sg.fen        = QString::fromStdString(m_ctrl->board().fen());
    sg.savedAtMove = static_cast<int>(m_moveLog.size());
    return sg;
}

bool PersistenceManager::saveGame(const QString& filePath, const QString& saveName)
{
    SavedGame sg;
    sg.saveName   = saveName.isEmpty() ? "Unnamed" : saveName;
    sg.dateTime   = QDateTime::currentDateTime().toString(Qt::ISODate);
    sg.playerMode = modeString();
    sg.difficulty = diffString();
    sg.humanColor = (m_ctrl->config().humanColor == WHITE) ? "White" : "Black";
    sg.moves      = m_moveLog;
    sg.fen        = QString::fromStdString(m_ctrl->board().fen());
    sg.savedAtMove = static_cast<int>(m_moveLog.size());

    if (!writeToFile(filePath, sg)) {
        emit saveError("Failed to write: " + filePath);
        return false;
    }
    addRecent(filePath);
    m_dirty = false;
    return true;
}

bool PersistenceManager::writeToFile(const QString& path, const SavedGame& sg) const
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);

    // ── JSON-like header block ────────────────────────────────────
    out << "{\n";
    out << "  \"save_name\": \"" << sg.saveName << "\",\n";
    out << "  \"date_time\": \"" << sg.dateTime << "\",\n";
    out << "  \"mode\": \""       << sg.playerMode << "\",\n";
    out << "  \"difficulty\": \"" << sg.difficulty << "\",\n";
    out << "  \"human_color\": \"" << sg.humanColor << "\",\n";
    out << "  \"saved_at_move\": " << sg.savedAtMove << ",\n";
    out << "  \"fen\": \""         << sg.fen << "\",\n";

    // DSA: Vector — serialise move list as JSON array
    out << "  \"moves\": [";
    for (size_t i = 0; i < sg.moves.size(); ++i) {
        out << "\"" << QString::fromStdString(sg.moves[i]) << "\"";
        if (i + 1 < sg.moves.size()) out << ",";
    }
    out << "]\n}\n";

    f.close();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Load Game
//  DSA: Stack — replay moves through controller rebuilds undo stack
// ─────────────────────────────────────────────────────────────────────────────

bool PersistenceManager::loadGame(const QString& filePath)
{
    SavedGame sg;
    if (!readFromFile(filePath, sg)) {
        emit loadError("Cannot read file: " + filePath);
        return false;
    }

    // Rebuild config
    GameConfig cfg;
    cfg.mode        = parseMode(sg.playerMode);
    cfg.difficulty  = parseDiff(sg.difficulty);
    cfg.humanColor  = (sg.humanColor == "White") ? WHITE : BLACK;

    // Start a fresh game with the saved config (resets board + undo stack)
    m_ctrl->newGame(cfg);

    // DSA: Stack — replay each move through the controller so both
    //              Board::history stack and ChessController::m_undoStack
    //              are rebuilt in the correct order
    clearMoveLog();
    for (const std::string& uci : sg.moves) {
        if (uci.size() < 4) continue;
        int fc = uci[0] - 'a', fr = uci[1] - '1';
        int tc = uci[2] - 'a', tr = uci[3] - '1';
        PieceType promo = NONE_PIECE;
        if (uci.size() == 5) {
            char p = uci[4];
            if (p == 'q') promo = QUEEN;
            else if (p == 'r') promo = ROOK;
            else if (p == 'b') promo = BISHOP;
            else if (p == 'n') promo = KNIGHT;
        }
        // Force-apply move even if it's the AI's turn
        Square from(fr, fc), to(tr, tc);
        m_ctrl->tryMove(from, to, promo);
    }

    addRecent(filePath);
    emit gameLoaded(sg);
    return true;
}

bool PersistenceManager::readFromFile(const QString& path, SavedGame& sg) const
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QString content = f.readAll();
    f.close();

    // Simple key-value parser for our JSON-like format
    auto extract = [&](const QString& key) -> QString {
        // Matches:  "key": "value"  or  "key": number
        QString search = "\"" + key + "\": ";
        int idx = content.indexOf(search);
        if (idx < 0) return {};
        idx += search.size();
        if (content[idx] == '"') {
            int end = content.indexOf('"', idx + 1);
            return content.mid(idx + 1, end - idx - 1);
        } else {
            // number
            int end = idx;
            while (end < content.size() && (content[end].isDigit() || content[end] == '-'))
                ++end;
            return content.mid(idx, end - idx);
        }
    };

    sg.saveName   = extract("save_name");
    sg.dateTime   = extract("date_time");
    sg.playerMode = extract("mode");
    sg.difficulty = extract("difficulty");
    sg.humanColor = extract("human_color");
    sg.fen        = extract("fen");
    sg.savedAtMove = extract("saved_at_move").toInt();

    // Parse moves array
    int arrStart = content.indexOf("\"moves\": [");
    int arrEnd   = content.indexOf("]", arrStart);
    if (arrStart >= 0 && arrEnd > arrStart) {
        QString arr = content.mid(arrStart + 10, arrEnd - arrStart - 10);
        // DSA: Vector — parse comma-separated quoted strings
        for (const QString& tok : arr.split(',')) {
            QString m = tok.trimmed().remove('"');
            if (!m.isEmpty()) sg.moves.push_back(m.toStdString());
        }
    }

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  PGN Export
// ─────────────────────────────────────────────────────────────────────────────

bool PersistenceManager::exportPGN(const QString& filePath) const
{
    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) return false;
    QTextStream out(&f);

    // PGN headers
    out << "[Event \"Chess GUI Game\"]\n";
    out << "[Site \"Chess GUI Qt\"]\n";
    out << "[Date \"" << QDateTime::currentDateTime().toString("yyyy.MM.dd") << "\"]\n";
    out << "[Round \"1\"]\n";
    out << "[White \"" << (m_ctrl->config().humanColor == WHITE ? "Human" : "AI") << "\"]\n";
    out << "[Black \"" << (m_ctrl->config().humanColor == BLACK ? "Human" : "AI") << "\"]\n";

    // Result
    GameStatus st = m_ctrl->status();
    QString result = "*";
    if (st == CHECKMATE) {
        // Side to move is the loser
        result = (m_ctrl->board().sideToMove == WHITE) ? "0-1" : "1-0";
    } else if (st == STALEMATE || st == DRAW_50_MOVE) {
        result = "1/2-1/2";
    }
    out << "[Result \"" << result << "\"]\n";
    out << "[FEN \"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\"]\n\n";

    // Replay moves through a scratch board to generate proper SAN
    // For simplicity we output UCI-like moves with move numbers
    // DSA: Vector — iterate over move log
    Board scratch;
    scratch.reset();
    int moveNum = 1;
    QString line;
    for (size_t i = 0; i < m_moveLog.size(); ++i) {
        bool isWhite = (i % 2 == 0);
        if (isWhite) {
            line += QString::number(moveNum) + ". ";
            ++moveNum;
        }
        line += QString::fromStdString(m_moveLog[i]) + " ";
        // Wrap at ~80 chars
        if (line.size() > 75) {
            out << line.trimmed() << "\n";
            line.clear();
        }
    }
    if (!line.trimmed().isEmpty()) out << line.trimmed() << "\n";
    out << result << "\n";

    f.close();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  PGN Import
// ─────────────────────────────────────────────────────────────────────────────

bool PersistenceManager::importPGN(const QString& filePath)
{
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit loadError("Cannot open PGN: " + filePath);
        return false;
    }
    QString content = f.readAll();
    f.close();

    // Strip header tags line by line, then tokenise
    QStringList tokens;
    for (const QString& line : content.split('\n')) {
        QString l = line.trimmed();
        if (l.startsWith('[') || l.isEmpty()) continue; // skip headers
        // Remove inline comments {…} — simple bracket scan
        QString clean;
        int depth = 0;
        for (QChar ch : l) {
            if (ch == '{') { ++depth; continue; }
            if (ch == '}') { --depth; continue; }
            if (depth == 0) clean += ch;
        }
        // Split on whitespace
        for (const QString& tok : clean.split(' ', Qt::SkipEmptyParts)) {
            // Skip move numbers ("1.", "23.") and results
            if (tok.contains('.') || tok == "1-0" || tok == "0-1" ||
                tok == "1/2-1/2" || tok == "*") continue;
            tokens << tok;
        }
    }

    // Start a fresh game
    GameConfig cfg;
    cfg.mode = PlayerMode::HumanVsHuman;
    m_ctrl->newGame(cfg);
    clearMoveLog();

    // DSA: Stack rebuilt by replaying through controller
    Board scratch;
    scratch.reset();
    MoveGenerator gen(scratch);

    for (const QString& tok : tokens) {
        // Each token is a move in SAN or UCI format
        std::string uci = tok.toStdString();
        // If it looks like UCI (e.g. "e2e4", "e7e8q")
        bool isUCI = (uci.size() >= 4 &&
                      uci[0] >= 'a' && uci[0] <= 'h' &&
                      uci[1] >= '1' && uci[1] <= '8' &&
                      uci[2] >= 'a' && uci[2] <= 'h' &&
                      uci[3] >= '1' && uci[3] <= '8');
        if (!isUCI) continue;  // skip SAN for now — UCI only

        int fc = uci[0]-'a', fr = uci[1]-'1';
        int tc = uci[2]-'a', tr = uci[3]-'1';
        PieceType promo = NONE_PIECE;
        if (uci.size() == 5) {
            char p = tolower(uci[4]);
            if (p=='q') promo=QUEEN; else if(p=='r') promo=ROOK;
            else if(p=='b') promo=BISHOP; else if(p=='n') promo=KNIGHT;
        }
        m_ctrl->tryMove(Square(fr,fc), Square(tr,tc), promo);
    }
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  FEN Export / Import
// ─────────────────────────────────────────────────────────────────────────────

QString PersistenceManager::exportFEN() const
{
    return QString::fromStdString(m_ctrl->board().fen());
}

bool PersistenceManager::importFEN(const QString& fen)
{
    Board b;
    if (!parseFEN(fen, b)) {
        emit loadError("Invalid FEN: " + fen);
        return false;
    }
    // Start new game with parsed board position
    // We apply it by restarting and using the board directly
    GameConfig cfg;
    cfg.mode = PlayerMode::HumanVsHuman;
    m_ctrl->newGame(cfg);
    clearMoveLog();
    // Copy the parsed board state into the controller's board
    // (We expose a setBoard() method — see ChessController)
    m_ctrl->setBoard(b);
    return true;
}

bool PersistenceManager::parseFEN(const QString& fen, Board& b) const
{
    b.clear();
    QStringList parts = fen.split(' ');
    if (parts.size() < 4) return false;

    // Piece placement
    int row = 7, col = 0;
    for (QChar ch : parts[0]) {
        if (ch == '/') { --row; col = 0; }
        else if (ch.isDigit()) col += ch.digitValue();
        else {
            Color c = ch.isUpper() ? WHITE : BLACK;
            PieceType t = NONE_PIECE;
            switch (ch.toLower().toLatin1()) {
                case 'p': t = PAWN;   break;
                case 'n': t = KNIGHT; break;
                case 'b': t = BISHOP; break;
                case 'r': t = ROOK;   break;
                case 'q': t = QUEEN;  break;
                case 'k': t = KING;   break;
                default: return false;
            }
            if (row < 0 || row > 7 || col < 0 || col > 7) return false;
            b.place(Square(row, col), Piece(t, c));
            ++col;
        }
    }

    // Side to move
    b.sideToMove = (parts[1] == "b") ? BLACK : WHITE;

    // Castling
    b.castlingRights[WHITE][0] = parts[2].contains('K');
    b.castlingRights[WHITE][1] = parts[2].contains('Q');
    b.castlingRights[BLACK][0] = parts[2].contains('k');
    b.castlingRights[BLACK][1] = parts[2].contains('q');

    // En passant
    if (parts[3] != "-" && parts[3].size() == 2) {
        int epc = parts[3][0].toLatin1() - 'a';
        int epr = parts[3][1].toLatin1() - '1';
        b.enPassantSquare = Square(epr, epc);
    }

    // Clocks
    if (parts.size() >= 5) b.halfMoveClock  = parts[4].toInt();
    if (parts.size() >= 6) b.fullMoveNumber = parts[5].toInt();

    b.zobristHash = b.computeZobrist();
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

QString PersistenceManager::diffString() const
{
    switch (m_ctrl->config().difficulty) {
        case EASY:   return "Easy";
        case MEDIUM: return "Medium";
        case HARD:   return "Hard";
        case EXPERT: return "Expert";
        default:     return "Medium";
    }
}

QString PersistenceManager::modeString() const
{
    switch (m_ctrl->config().mode) {
        case PlayerMode::HumanVsHuman: return "HvH";
        case PlayerMode::HumanVsAI:   return "HvAI";
        case PlayerMode::AIVsHuman:   return "AIvH";
        default:                       return "HvAI";
    }
}

Difficulty PersistenceManager::parseDiff(const QString& s) const
{
    if (s == "Easy")   return EASY;
    if (s == "Hard")   return HARD;
    if (s == "Expert") return EXPERT;
    return MEDIUM;
}

PlayerMode PersistenceManager::parseMode(const QString& s) const
{
    if (s == "HvH")  return PlayerMode::HumanVsHuman;
    if (s == "AIvH") return PlayerMode::AIVsHuman;
    return PlayerMode::HumanVsAI;
}

QStringList PersistenceManager::recentSaves() const
{
    return m_recentSaves;
}

void PersistenceManager::addRecent(const QString& path)
{
    m_recentSaves.removeAll(path);
    m_recentSaves.prepend(path);
    while (m_recentSaves.size() > MAX_RECENT)
        m_recentSaves.removeLast();

    QSettings s("ChessQt", "ChessGUI");
    s.setValue("recentSaves", m_recentSaves);
}
