% Automatically generated by BMC, the braille music compiler
\version "2.14.2"
\include "articulate.ly"
music =
  <<
    \new Staff {
      c''4 \times 2/3 { e''8 f'' g'' } \times 2/3 { f'' \times 2/3 { e''16 f'' e'' } d''8 } c''4 | % 1
    }
  >>

\score {
  \music
  \layout { }
}
\score {
  \unfoldRepeats \articulate \music
  \midi { }
}
