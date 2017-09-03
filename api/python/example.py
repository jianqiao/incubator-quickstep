from quickstep import Quickstep

qs = Quickstep('/Users/jianqiao/Desktop/incubator-quickstep/build/Debug/quickstep_client')

print 'Executing "SELECT * FROM r" ...'
print '--'
print qs.execute('SELECT * FROM r')

print 'Executing "COPY SELECT * FROM r TO stdout WITH (FORMAT CSV, HEADER FALSE)" ...'
print '--'
print qs.execute('COPY SELECT * FROM r TO stdout WITH (FORMAT CSV, HEADER FALSE)')
