SELECT p.name, p.id
FROM Trainer t, CatchedPokemon cp, Pokemon p
WHERE t.id = cp.owner_id AND cp.pid = p.id AND t.hometown = 'Sangnok City'
ORDER BY p.id