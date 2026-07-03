# การสร้างไฟล์ Entry Point (Pulsar-style) สำหรับ NP Compiler

ในการเขียน package เสริมเพื่อให้สามารถ import ใช้งานในภาษา NP ได้อย่างสะดวก เช่น:
```np
import "pulsar" as ps
ps.greet("World")
```

ภาษา NP Compiler ใช้โครงสร้าง **Directory Entry Point Resolution** โดยเมื่อมีการเขียน `import "pulsar"` ตัว compiler จะค้นหาไฟล์ตามลำดับดังนี้:
1. `pulsar/mod.np`
2. `pulsar/main.np`
3. `pulsar/index.np`

---

## 1. วิธีทำไฟล์ Entry Point (`mod.np`)

ให้สร้างโฟลเดอร์ชื่อ `pulsar` และสร้างไฟล์ชื่อ `mod.np` ไว้ด้านใน เพื่อให้เป็นไฟล์หลักของ package:

### โครงสร้างโฟลเดอร์:
```
pulsar/
├── mod.np          <-- Entry Point (ไฟล์หลักที่รวมและเปิดให้เรียกใช้)
├── math.np         <-- โค้ดย่อยส่วนคำนวณ
└── utils.np        <-- โค้ดย่อยทั่วไป
```

### ตัวอย่างไฟล์ `pulsar/mod.np`:
```np
# โหลดไฟล์ย่อยภายในแพ็กเกจเข้ามาไว้ใน namespace เดียวกัน
import "pulsar/math.np"
import "pulsar/utils.np"

# หรือจะเขียนฟังก์ชันหลักเพิ่มเติมไว้ในไฟล์นี้โดยตรงก็ได้
fn greet(string name):
    print("Welcome to Pulsar, " + name)
```

---

## 2. วิธีใช้งานผ่าน Namespace Alias

เมื่อผู้ใช้นำไปเขียนในไฟล์โปรเจกต์ของตนเอง (`main.np`):

```np
# นำเข้าผ่าน alias 'ps'
import "pulsar" as ps

# เรียกใช้ฟังก์ชันจาก mod.np โดยตรง
ps.greet("Kit")

# เรียกใช้ฟังก์ชันที่ประกาศใน pulsar/math.np (ที่ถูก merge เข้ามาใน mod.np แล้ว)
int val = ps.add(10, 20)
print(val)
```

---

## 3. ตัวอย่างการสร้าง Remote Package

เมื่อคุณต้องการนำขึ้น GitHub เพื่อให้ดาวน์โหลดผ่านระบบ Remote Import อัตโนมัติ:

1. นำโฟลเดอร์ย่อยหรือไฟล์ทั้งหมดไปไว้ที่ root ของ Repository บน GitHub เช่น `github.com/peeb01/pulsar`
2. ตรวจสอบให้มั่นใจว่าไฟล์หลักชื่อ `mod.np` อยู่ที่ root ของ Repo (เมื่อแตก zip แล้วจะถูกค้นพบทันที)
3. เรียกใช้งานในโปรเจกต์:
   ```np
   import "github.com/peeb01/pulsar" as ps
   ps.greet("Developer")
   ```
