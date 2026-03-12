# Chess AI - CMPM 123 - Noah Baron

This AI was particularly difficult to implement. I used the given evaluate.h function with some very slight modifications to allow my AI to evaluate each board state. The main difficulty and challenge was getting negamax and updateAI to function properly. The biggest issues I had were either the AI not making any moves, or moving to incorrect type of piece. This led me to needing to make a second generateAll moves where the state is passed in as an argument. Before this, the AI was only using the current visual state of the board, and not the branches that it was supposed to be making. THis caused many bugs and weird movement issues, however I created an AI specific generateAllMoves funtion which fixed these issues. What came next were slight optimization fixes to get the AI to run a little faster, but I haven't been able to make it reach farther than a depth of 3. My final bug involved engame states and pawn movement, where pawns in specific spots would cause the AI to crash because it would look out of bounds. I was able to solve this by putting in more guard checks for the pawn movement generation, and this bug was solved.

Maximum depth acheived currently is 3.

This AI plays fine. I am not a very good chess player, but on occasion I am able to beat it. I would estimate it being around the 1000-1300 range, which is around my skill level. I would love to optimize this AI further so that it can go a deeper depth, and play a much better game of chess.

This was developed on Windows 10. 
