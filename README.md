# LangDict

Responsible for reading the Linux provided dictionaries from '/usr/share/dict/' and creating
dictionaries out of it (or any other file that contains a list of words with each line 
containing one word). Those dictionaries contain a collection of words and their near neighbours,
defined through the Levenshtein distance (which can be specified through the additional parameter
--levenshtein).

Through that we can create complex replacement dictionaries for the generation of our
error detection framework.

## Build instructions

```bash
make lang_dict
```

## Usage

```bash
./lang_dict 
  --input=<PATH/TO/INPUT_WORDS>
  --output=<PATH/TO/OUTPUT_FILE>
  --archaic=<PATH/TO/ARCHAIC_WORDS>
  --levenshtein=1
```

For single language:

```bash
./lang_dict --output=<PATH/TO/FILE.pkl> [--archaic=FILE] --input=<LIST OF FILES>
```

Example:

```bash
./lang_dict --output=./langs/en/en_US.json --input=/usr/share/dict/american-english-insane
```
