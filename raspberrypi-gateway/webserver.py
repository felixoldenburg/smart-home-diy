import flask
from flask import request
import sqlite3

app = flask.Flask(__name__)
app.config["DEBUG"] = True

@app.route('/tmp', methods=['GET'])
def getTmp():
    conn = createDBConnection()
    manual, tmp = readSettings(conn)
    return str(tmp)

@app.route('/tmp', methods=['PUT'])
def setTmp():
    conn = createDBConnection()
    setTemp(conn, request.args.get("value"))
    return "OK"

def createDBConnection():
    conn = None
    try:
        conn = sqlite3.connect("temp.db", detect_types=sqlite3.PARSE_DECLTYPES)
    except Error as e:
        print(e)

    return conn

def setTemp(conn, tmp):
    with conn:
        c = conn.cursor()
        c.execute("UPDATE settings SET tmp=? WHERE id=1", (tmp,))

def setMode(conn, manual):
    with conn:
        c = conn.cursor()
        c.execute("UPDATE settings SET manual=? WHERE id=1", (manual,))

def readSettings(conn):
    c = conn.cursor()
    c.executescript("""
        create table if not exists settings(
            id,
            manual,
            tmp
        );
    
        create unique index if not exists IdIndex on settings ( id ) ;
    
        insert or ignore into settings(id, manual, tmp)
        values (
            1,
            1,
            21
        );
        """)
    
    c.execute("select * from settings where id=1")
    row = c.fetchone()
    manual, tmp = row[1], row[2]
    return manual, tmp

def main():
    conn = createDBConnection()
    manual, tmp = readSettings(conn)
    print(manual)
    print(tmp)
    setTemp(conn, 25)
    manual, tmp = readSettings(conn)
    print(manual)
    print(tmp)
    app.run(host='0.0.0.0')

if __name__ == '__main__':
    main()

