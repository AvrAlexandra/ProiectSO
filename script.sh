#!/bin/bash

# Verifică dacă script-ul a fost apelat cu un singur argument
if [ "$#" -ne 1 ]; then
    echo "Utilizare: $0 <c>"
    exit 1
fi

# Caracterul primit ca argument
character="$1"

# Contor pentru propozițiile corecte
correct_sentences=0

# Citirea linie cu linie până la end-of-file (CTRL+D)
while IFS= read -r line; do
    # Verifică dacă linia îndeplinește condițiile date
    if [[ "$line" =~ ^[[:upper:]][[:alnum:][:space:],.!?]*[[:upper:][:digit:][:space:]]?[.!?]$ && "$line" != *,* ]]; then
        # Verifică dacă linia conține caracterul specificat
        if [[ "$line" =~ $character ]]; then
            ((correct_sentences++))
        fi
    fi
done

# Afișează numărul de propoziții corecte
echo "$correct_sentences"
