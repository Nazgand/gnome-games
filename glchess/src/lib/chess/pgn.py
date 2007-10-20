"""
Implement a PGN reader/writer.

See http://www.chessclub.com/help/PGN-spec
"""

__author__ = 'Robert Ancell <bob27@users.sourceforge.net>'
__license__ = 'GNU General Public License Version 2'
__copyright__ = 'Copyright 2005-2006  Robert Ancell'

"""
; Example PGN file

[Event "F/S Return Match"]
[Site "Belgrade, Serbia JUG"]
[Date "1992.11.04"]
[Round "29"]
[White "Fischer, Robert J."]
[Black "Spassky, Boris V."]
[Result "1/2-1/2"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 8. c3
O-O 9. h3 Nb8 10. d4 Nbd7 11. c4 c6 12. cxb5 axb5 13. Nc3 Bb7 14. Bg5 b4 15.
Nb1 h6 16. Bh4 c5 17. dxe5 Nxe4 18. Bxe7 Qxe7 19. exd6 Qf6 20. Nbd2 Nxd6 21.
Nc4 Nxc4 22. Bxc4 Nb6 23. Ne5 Rae8 24. Bxf7+ Rxf7 25. Nxf7 Rxe1+ 26. Qxe1 Kxf7
27. Qe3 Qg5 28. Qxg5 hxg5 29. b3 Ke6 30. a3 Kd6 31. axb4 cxb4 32. Ra5 Nd5 33.
f3 Bc8 34. Kf2 Bf5 35. Ra7 g6 36. Ra6+ Kc5 37. Ke1 Nf4 38. g3 Nxh3 39. Kd2 Kb5
40. Rd6 Kc5 41. Ra6 Nf2 42. g4 Bd3 43. Re6 1/2-1/2
"""

RESULT_INCOMPLETE = '*'
RESULT_WHITE_WIN  = '1-0'
RESULT_BLACK_WIN  = '0-1'
RESULT_DRAW       = '1/2-1/2'

"""The required tags in a PGN file (the seven tag roster, STR)"""
TAG_EVENT  = 'Event'
TAG_SITE   = 'Site'
TAG_DATE   = 'Date'
TAG_ROUND  = 'Round'
TAG_WHITE  = 'White'
TAG_BLACK  = 'Black'
TAG_RESULT = 'Result'

"""Optional tags"""
TAG_TIME         = 'Time'
TAG_FEN          = 'FEN'
TAG_WHITE_TYPE   = 'WhiteType'
TAG_WHITE_ELO    = 'WhiteElo'
TAG_BLACK_TYPE   = 'BlackType'
TAG_BLACK_ELO    = 'BlackElo'
TAG_TIME_CONTROL = 'TimeControl'
TAG_TERMINATION  = 'Termination'

# Values for the WhiteType and BlackType tag
PLAYER_HUMAN     = 'human'
PLAYER_AI        = 'program'

# Values for the Termination tag
TERMINATE_ABANDONED        = 'abandoned'
TERMINATE_ADJUDICATION     = 'adjudication'
TERMINATE_DEATH            = 'death'
TERMINATE_EMERGENCY        = 'emergency'
TERMINATE_NORMAL           = 'normal'
TERMINATE_RULES_INFRACTION = 'rules infraction'
TERMINATE_TIME_FORFEIT     = 'time forfeit'
TERMINATE_UNTERMINATED     = 'unterminated'

# Comments are bounded by ';' to '\n' or '{' to '}'
# Lines starting with '%' are ignored and are used as an extension mechanism
# Strings are bounded by '"' and '"' and quotes inside the strings are escaped with '\"'

class Error(Exception):
    """PGN exception class"""
    
    __errorType = 'Unknown'
    
    # Text description of the error
    description = ''
    
    # The file being opened
    fileName = ''
    
    def __init__(self, description = '', fileName = ''):
        """
        """
        self.description = description
        self.fileName = fileName
        Exception.__init__(self)
        
    def __str__(self):
        if self.fileName != '':
            string = self.fileName + ': '
        else:
            string = ''
        string += self.description
        return string

class PGNToken:
    """
    """
    
    # Token types
    LINE_COMMENT = 'Line comment'
    COMMENT      = 'Comment'
    ESCAPED      = 'Escaped data'
    PERIOD       = 'Period'
    TAG_START    = 'Tag start'
    TAG_END      = 'Tag end'
    STRING       = 'String'
    SYMBOL       = 'Symbol'
    RAV_START    = 'RAV start'
    RAV_END      = 'RAV end'
    XML_START    = 'XML start'
    XML_END      = 'XML end'
    NAG          = 'NAG'
    type = None

    SYMBOL_START_CHARACTERS = '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ' + '*'
    SYMBOL_CONTINUATION_CHARACTERS = SYMBOL_START_CHARACTERS + '_+#=:-' + '/' # Not in spec but required from game draw and incomplete
    NAG_CONTINUATION_CHARACTERS = '0123456789'
    
    data = None
    
    lineNumber = -1
    characterNumber = -1
    
    def __init__(self, lineNumber, characterNumber, tokenType, data = None):
        """
        """
        self.type = tokenType
        self.data = data
        self.lineNumber = lineNumber
        self.characterNumber = characterNumber
        
    def __str__(self):
        string = self.type
        if self.data is not None:
            string += ': ' + self.data
        return string
    
    def __repr__(self):
        return self.__str__()
    
class PGNParser:
    """
    """
    
    __inComment = False
    __comment = ''
    __startOffset = -1
    
    def __init__(self):
        self.tokens = {' ':  (None,                  1),
                       '\t': (None,                  1),
                       '\n': (None,                  1),
                       ';':  (PGNToken.LINE_COMMENT, self.__lineComment),
                       '{':  (PGNToken.LINE_COMMENT, self.__collectComment),
                       '.':  (PGNToken.PERIOD,       1),
                       '[':  (PGNToken.TAG_START,    1),
                       ']':  (PGNToken.TAG_END,      1),
                       '"':  (PGNToken.STRING,       self.__extractPGNString),
                       '(':  (PGNToken.RAV_START,    1),
                       ')':  (PGNToken.RAV_END,      1),
                       '<':  (PGNToken.XML_START,    1),
                       '>':  (PGNToken.XML_END,      1),
                       '$':  (PGNToken.NAG,          self.__extractNAG),
                       '*':  (PGNToken.SYMBOL,       1)}

        for c in '0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ':
            self.tokens[c] = (PGNToken.SYMBOL, self.__extractSymbol)
            
    def __lineComment(self, data):
        return (data, len(data))

    def __collectComment(self, data):
        index = data.find('}')
        if index < 0:
           self.__inComment = True
           index = data.find('\n')
           self.__comment = self.__comment + data[:index+1]
           return (data[:index+1], index+1)
        else:
            self.__comment = self.__comment + data[:index+1]
            self.__inComment = False
                 
        return (self.__comment, index+1)
            
    def __extractSymbol(self, data):
        for offset in xrange(1, len(data)):
            if PGNToken.SYMBOL_CONTINUATION_CHARACTERS.find(data[offset]) < 0:
                return (data[:offset], offset)

        return (data, len(data))

    def __extractNAG(self, data):
        index = PGNToken.NAG_CONTINUATION_CHARACTERS.find(data[1])
        for offset in xrange(1, len(data)):            
            if PGNToken.NAG_CONTINUATION_CHARACTERS.find(data[offset]) < 0:
            	#FIXME: Should be at lest one character and less than $255
                return (data[:offset], offset)

        return (data, len(data))
    
    def __extractPGNString(self, data):
        #"""Extract a PGN string.
        
        #'data' is the data to extract the string from (string). It must start with a quote character '"'.
        
        #Return a tuple containing the first PGN string and the number of characters of data it required.
        #e.g. '"Mike \"Dog\" Smith"' -> ('Mike "Dog" Smith', 20).
        #If no string is found a Error is raised.
        #"""
        if data[0] != '"':
            raise Error('PGN string does not start with "')
        
        for offset in xrange(1, len(data)):
            c = data[offset]
            escaped = (c == '\\')
            if c == '"' and escaped is False:
                pgnString = data[1:offset]
                pgnString.replace('\\"', '"')
                pgnString.replace('\\\\', '\\')
                return (pgnString, offset + 1)

        raise Error('Unterminated PGN string')
            
    def parseLine(self, line, lineNumber):
        """TODO
        
        Return an array of tokens extracted from the line.
        """
        # Ignore line if contains escaped data
        if line[0] == '%':
            return [PGNToken(lineNumber, self.__startOffset, PGNToken.ESCAPED, line[1:])]
        
        offset = 0
        tokens = []
        while offset < len(line):
            c = line[offset]

            if self.__inComment:
                try:
                    (tokenType, length) = self.tokens['{']
                except KeyError:
                    raise Error('Unknown character %s' % repr(c))
            else:
                self.__comment = ''
                try:
                    (tokenType, length) = self.tokens[c]
                except KeyError:
                    raise Error('Unknown character %s' % repr(c))

            startOffset = offset
            if type(length) is int:
                data = line[offset:offset+length]
                offset += length
            else:
                (data, o) = length(line[offset:])
                offset += o
            
            if tokenType is not None and not self.__inComment:
                tokens.append(PGNToken(lineNumber, startOffset, tokenType, data))

        return tokens
            
    def endParse(self):
        pass
    
class PGNGameParser:
    """
    """
    
    STATE_IDLE       = 'IDLE'
    STATE_TAG_NAME   = 'TAG_NAME'
    STATE_TAG_VALUE  = 'TAG_VALUE'
    STATE_TAG_END    = 'TAG_END'
    STATE_MOVETEXT   = 'MOVETEXT'
    STATE_RAV        = 'RAV'
    STATE_XML        = 'XML'
    __state = STATE_IDLE
    
    # The game being assembled
    __game = None
    
    # The tag being assembled
    __tagName = None
    __tagValue = None
    
    # The move number being decoded
    __expectedMoveNumber = 0
    __prevTokenIsMoveNumber = False
    
    __currentMoveNumber = 0

    # The last white move
    __whiteMove = None
    
    # The Recursive Annotation Variation (RAV) stack
    __ravDepth = 0
    
    def __parseTokenMovetext(self, token):
        """
        """
        if token.type is PGNToken.RAV_START:
            self.__ravDepth += 1
            # FIXME: Check for RAV errors
            return
                
        elif token.type is PGNToken.RAV_END:
            self.__ravDepth -= 1
            # FIXME: Check for RAV errors
            return

        # Ignore tokens inside RAV
        if self.__ravDepth != 0:
            return
                
        if token.type is PGNToken.PERIOD:
            if self.__prevTokenIsMoveNumber is False:
                raise Error('Unexpected period on line %i:%i' % (token.lineNumber, token.characterNumber))

        elif token.type is PGNToken.SYMBOL:
            # See if this is a game terminate
            if token.data == RESULT_INCOMPLETE or \
               token.data == RESULT_WHITE_WIN or \
               token.data == RESULT_BLACK_WIN or \
               token.data == RESULT_DRAW:
                game = self.__game
                self.__game = None
                
                return game
            
            # Otherwise it is a move number or a move
            else:
                # See if this is a move number or a SAN move
                try:
                    moveNumber = int(token.data)
                except ValueError:
                    self.__prevTokenIsMoveNumber = False
                    if self.__whiteMove is None:
                        self.__whiteMove = token.data
                        move = PGNMove()
                        move.number = self.__currentMoveNumber
                        move.move = self.__whiteMove
                        self.__game.addMove(move)
                        self.__currentMoveNumber += 1

                    else:
                        move = PGNMove()
                        move.move = token.data
                        move.number = self.__currentMoveNumber
                        self.__game.addMove(move)
                        self.__whiteMove = None
                        self.__currentMoveNumber += 1
                        self.__expectedMoveNumber += 1
                else:
                    self.__prevTokenIsMoveNumber = True
                    if moveNumber != self.__expectedMoveNumber:
                        raise Error('Expected move number %i, got %i on line %i:%i' % (self.__expectedMoveNumber, moveNumber, token.lineNumber, token.characterNumber))

        elif token.type is PGNToken.NAG:
            pass
                
        else:
            raise Error('Unknown token %s in movetext on line %i:%i' % (str(token.type), token.lineNumber, token.characterNumber))
    
    def parseToken(self, token):
        """TODO
        
        Return a game object if a game is complete otherwise None.
        """
        # Ignore all comments at any time
        if token.type is PGNToken.LINE_COMMENT or token.type is PGNToken.COMMENT:
            if self.__currentMoveNumber > 0:
                move = self.__game.getMove(self.__currentMoveNumber)
                move.comment = token.data[1:-1]
            return None

        if token.type is PGNToken.NAG:
            if self.__currentMoveNumber > 0:
                move = self.__game.getMove(self.__currentMoveNumber)
                move.nag = token.data               
            return None
            
        if self.__state is self.STATE_IDLE:
            if self.__game is None:
                self.__game = PGNGame()
                
            if token.type is PGNToken.TAG_START:
                self.__state = self.STATE_TAG_NAME
                return

            elif token.type is PGNToken.SYMBOL:
                self.__expectedMoveNumber = 1
                self.__whiteMove = None
                self.__prevTokenIsMoveNumber = False
                self.__ravDepth = 0
                self.__state = self.STATE_MOVETEXT
                
            elif token.type is PGNToken.ESCAPED:
                pass
            
            else:
                raise Error('Unexpected token %s on line %i:%i' % (str(token.type), token.lineNumber, token.characterNumber))

        if self.__state is self.STATE_TAG_NAME:
            if token.type is PGNToken.SYMBOL:
                self.__tagName = token.data
                self.__state = self.STATE_TAG_VALUE
            else:
                raise Error('Not a valid file')

        elif self.__state is self.STATE_TAG_VALUE:
            if token.type is PGNToken.STRING:
                self.__tagValue = token.data
                self.__state = self.STATE_TAG_END
            else:
                raise Error('Not a valid file')
            
        elif self.__state is self.STATE_TAG_END:
            if token.type is PGNToken.TAG_END:
                self.__game.setTag(self.__tagName, self.__tagValue)
                self.__state = self.STATE_IDLE
            else:
                raise Error('Not a valid file')

        elif self.__state is self.STATE_MOVETEXT:
            game = self.__parseTokenMovetext(token)
            if game is not None:
                self.__state = self.STATE_IDLE
                return game
                
    def complete(self):
        """
        """
        self.__state = self.STATE_IDLE
        return self.__game
        
class PGNMove:
    """
    """
    #
    move       = ''
    
    #
    comment    = ''

    #
    nag        = ''

class PGNGame:
    """
    """

    # The seven tag roster in the required order (REFERENCE)
    __strTags = [TAG_EVENT, TAG_SITE, TAG_DATE, TAG_ROUND, TAG_WHITE, TAG_BLACK, TAG_RESULT]

    # The tags in this game
    __tagsByName = None
    
    __moves = None
    
    def __init__(self):
        # Set the default STR tags
        self.__tagsByName = {}
        self.setTag(TAG_EVENT, '?')
        self.setTag(TAG_SITE, '?')
        self.setTag(TAG_DATE, '????.??.??')
        self.setTag(TAG_ROUND, '?')
        self.setTag(TAG_WHITE, '?')
        self.setTag(TAG_BLACK, '?')
        self.setTag(TAG_RESULT, '*')
        
        self.__moves = []
        
    def getLines(self):
    
        lines = []
        
        # Get the names of the non STR tags
        otherTags = list(set(self.__tagsByName).difference(self.__strTags))

        # Write seven tag roster and the additional tags
        for name in self.__strTags + otherTags:
            value = self.__tagsByName[name]
            lines.append('['+ name + ' ' + self.__makePGNString(value) + ']')

        lines.append('')
        
        # Insert numbers in-between moves
        tokens = []
        moveNumber = 0
        for m in self.__moves:
            if moveNumber % 2 == 0:
                tokens.append('%i.' % (moveNumber / 2 + 1))
            moveNumber += 1
            tokens.append(m.move)
            if m.nag != '':
                tokens.append(m.nag)
            if m.comment != '':
                tokens.append('{' + m.comment + '}')
                
        # Add result token to the end
        tokens.append(self.__tagsByName[TAG_RESULT])

        # Print moves keeping the line length to less than 256 characters (PGN requirement)
        line = ''
        for t in tokens:
            if line == '':
                x = t
            else:
                x = ' ' + t
            if len(line) + len(x) >= 80: #>= 256:
                lines.append(line)
                line = t
            else:
                line += x

        lines.append(line)
        return lines
        
    def setTag(self, name, value):
        """Set a PGN tag.
        
        'name' is the name of the tag to set (string).
        'value' is the value to set the tag to (string) or None to delete the tag.
        
        Tag names cannot contain whitespace.
        
        Deleting a tag that does not exist has no effect.
        
        Deleting a STR tag or setting one to an invalid value will raise an Error exception.
        """
        if self.__isValidTagName(name) is False:
            raise Error('%s is an invalid tag name' % str(name))
        
        # If no value delete
        if value is None:
            # If is a STR tag throw an exception
            if self.__strTags.has_key(name):
                raise Error('%s is a PGN STR tag and cannot be deleted' % name)
            
            # Delete the tag
            try:
                self.__strTags.pop(name)
            except KeyError:
                pass
        
        # Otherwise set the tag to the new value
        else:
            # FIXME: Validate if it is a STR tag
            
            self.__tagsByName[name] = value
    
    def getTag(self, name, default = None):
        """Get a PGN tag.
        
        'name' is the name of the tag to get (string).
        'default' is the default value to return if this valid is missing (user-defined).
        
        Return the value of the tag (string) or the default if the tag does not exist.
        """
        try:
            return self.__tagsByName[name]
        except KeyError:
            return default
        
    def addMove(self, move):
        self.__moves.append(move)

    def getMove(self, moveNumber):
        return self.__moves[moveNumber - 1]
    
    def getMoves(self):
        return self.__moves

    def __str__(self):
        
        string = ''
        for tag, value in self.__tagsByName.iteritems():
            string += tag + ' = ' + value + '\n'
        string += '\n'
        
        number = 1
        for move in self.__moves:
            string += '%3i. ' % number + str(move[0]) + ' ' + str(move[1]) + '\n'
            number += 1
            
        return string
    
    # Private methods    
    def __makePGNString(self, string):
        """Make a PGN string.
        
        'string' is the string to convert to a PGN string (string).
        
        All characters are valid and quotes are escaped with '\"'.
        
        Return the string surrounded with quotes. e.g. 'Mike "Dog" Smith' -> '"Mike \"Dog\" Smith"'
        """
        pgnString = string
        pgnString.replace('"', '\\"')
        return '"' + pgnString + '"'    

    def __isValidTagName(self, name):
        """Valid a PGN tag name.
        
        'name' is the tag name to validate (string).
        
        Tags can only contain the characters, a-Z A-Z and _.
        
        Return True if this is a valid tag name otherwise return False.
        """
        if name is None or len(name) == 0:
            return False

        validCharacters = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_'
        for c in name:
            if validCharacters.find(c) < 0:
                return False
        return True

class PGN:
    """
    """
    
    __games = None

    def __init__(self, fileName = None, maxGames = None):
        """Create a PGN reader/writer.
        
        'fileName' is the file to load the PGN from or None to generate an empty PGN file.
        'maxGames' is the maximum number of games to load from the file or None
                   to load the whole file. (int, Only applicable if a filename is supplied).
        """       
        self.__games = []

        if fileName is not None:
            self.__load(fileName, maxGames)
            
    def addGame(self):
        """Add a new game to the PGN file.
        
        Returns the PGNGame instance to modify"""
        game = PGNGame()
        self.__games.append(game)
        return game
    
    def getGame(self, index):
        """Get a game from the PGN file.
        
        'index' is the game index to get (integer, 0-N).
        
        Return this PGN game or raise an IndexError if no game with this index.
        """
        return self.__games[index]
    
    def save(self, fileName):
        """Save the PGN file.
        
        'fileName' is the name of the file to save to.
        """
        f = file(fileName, 'w')
        # FIXME: Set the newline characters to the correct type?
        
        # Sign it from glChess
        f.write('; PGN saved game generated by glChess\n')
        f.write('; http://glchess.sourceforge.net\n')

        for game in self.__games:
            f.write('\n')
            for line in game.getLines():
                f.write(line + '\n')
            
        f.close()
        
    def __len__(self):
        return len(self.__games)
        
    def __getitem__(self, index):
        return self.__games[index]

    def __getslice__(self, start, end):
        return self.__games[start:end]
    
    # Private methods

    def __load(self, fileName, maxGames = None):
        """
        """
        # Convert the file into PGN tokens
        f = file(fileName, 'r')
        p = PGNParser()
        gp = PGNGameParser()
        lineNumber = 1
        gameCount = 0
        games = []
        while True:
            # Read a line from the file
            line = f.readline()
            if line == '':
                game = gp.complete()
                if game is not None:
                    self.__games.append(game)
                break

            # Parse the line into tokens
            tokens = p.parseLine(line, lineNumber)

            # Decode the tokens into PGN games
            for token in tokens:
                game = gp.parseToken(token)
                
                # Store this game and stop if only required to parse a certain number
                if game is not None:
                    gp = PGNGameParser()
                    self.__games.append(game)
                    gameCount += 1
                        
                    if maxGames is not None and gameCount >= maxGames:
                        break

            # YUCK... FIXME
            if maxGames is not None and gameCount >= maxGames:
                break
            
            lineNumber += 1
            
        # Must be at least one game in the PGN file
        if len(self.__games) == 0:
            raise Error('Empty PGN file')

        # Tidy up
        p.endParse()
        f.close()

if __name__ == '__main__':
    def test(fileName, maxGames = None):
        p = PGN(fileName, maxGames)
        number = 1
        games = p[:]
        for game in games:
            print 'Game ' + str(number)
            print game
            print 
            number += 1

    test('example.pgn')
    test('rav.pgn')
    test('wolga-benko.pgn', 3)

    p = PGN('example.pgn')
    p.save('out.pgn')
