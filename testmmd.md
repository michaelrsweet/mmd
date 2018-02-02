---
title: Mini-Markdown Test Document
...

All heading levels are supported from 1 to 6, using both the ATX and Setext
forms.  As an indented code block:

    # Heading 1
    ## Heading 2
    ### Heading 3
    #### Heading 4
    ##### Heading 5
    ###### Heading 6

    Setext Heading 1
    ================

    Setext Heading 2
    ----------------

As block headings:

# Heading 1
## Heading 2
### Heading 3
#### Heading 4
##### Heading 5
###### Heading 6

Setext Heading 1
================

Setext Heading 2
----------------

And block quotes:

> # BQ Heading 1
> ## BQ Heading 2
> ### BQ Heading 3
> #### BQ Heading 4
> ##### BQ Heading 5
> ###### BQ Heading 6
>
> Setext Heading 1
> ================
>
> Setext Heading 2
> ----------------

And ordered lists:

1. First item.

2. Second item.

3. Third item with very long text that wraps
   across multiple lines.

   With a secondary paragraph associated with
   the third item.

And unordered lists:

- First item.

+ Second item.

* Third item.

Code block:

```
#include <stdio.h>

int main(void)
{
  puts("Hello, World!");
  return (0);
}
```

Link to [mmd web site](https://michaelrsweet.github.io/mmd).

Link to [Heading 1](@).

Link to [`Heading 2`](@).

Autolink to <https://michaelrsweet.github.io/mmd>.

Image: ![Michael R Sweet](https://michaelrsweet.github.io/apple-touch-icon.png)

This sentence contains *Emphasized Text*, **Bold Text**, and `Code Text` for
testing the MMD parser.  The `<mmd.h>` header file.

\(Escaped Parenthesis)

\(*Emphasized Parenthesis*)

\(**Boldface Parenthesis**)

\(`Code Parenthesis`)

Tables as code:

    | Heading 1 | Heading 2 | Heading 3 |
    | --------- | --------- | --------- |
    | Cell 1,1  | Cell 1,2  | Cell 1,3  |
    | Cell 2,1  | Cell 2,2  | Cell 2,3  |
    | Cell 3,1  | Cell 3,2  | Cell 3,3  |

Table with leading/trailing pipes:

| Heading 1 | Heading 2 | Heading 3 |
| --------- | --------- | --------- |
| Cell 1,1  | Cell 1,2  | Cell 1,3  |
| Cell 2,1  | Cell 2,2  | Cell 2,3  |
| Cell 3,1  | Cell 3,2  | Cell 3,3  |

Table without leading/trailing pipes:

Heading 1 | Heading 2 | Heading 3
--------- | --------- | ---------
Cell 1,1  | Cell 1,2  | Cell 1,3
Cell 2,1  | Cell 2,2  | Cell 2,3
Cell 3,1  | Cell 3,2  | Cell 3,3

Table with alignment:

Left Alignment | Center Alignment | Right Alignment
:-------- | :-------: | --------:
Cell 1,1  | Cell 1,2  |        1
Cell 2,1  | Cell 2,2  |       12
Cell 3,1  | Cell 3,2  |      123

Table in block quote:

> Heading 1 | Heading 2 | Heading 3
> --------- | --------- | ---------
> Cell 1,1  | Cell 1,2  | Cell 1,3
> Cell 2,1  | Cell 2,2  | Cell 2,3
> Cell 3,1  | Cell 3,2  | Cell 3,3
