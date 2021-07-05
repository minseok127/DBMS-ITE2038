SELECT t.name
FROM Pokemon p, Evolution e, CatchedPokemon cp, Trainer t
WHERE p.id = e.after_id AND cp.owner_id = t.id AND p.id = cp.pid AND
       e.after_id NOT IN (
         SELECT e2.before_id
         FROM Evolution e2 )
ORDER BY t.name