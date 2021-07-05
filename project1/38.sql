SELECT p.name
FROM Pokemon p, Evolution e
WHERE p.id = e.after_id AND
       e.after_id NOT IN (
         SELECT e2.before_id
         FROM Evolution e2 )
ORDER BY p.name