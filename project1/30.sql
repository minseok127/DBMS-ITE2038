SELECT e1.before_id AS 'ID', p1.name AS '1st name', p2.name AS ' 2nd name', p3.name AS '3rd name'
FROM Evolution e1, Evolution e2, Pokemon p1, Pokemon p2, Pokemon p3
WHERE e1.after_id = e2.before_id AND e1.before_id = p1.id AND e2.before_id = p2.id AND
      e2.after_id = p3.id
ORDER BY e1.before_id