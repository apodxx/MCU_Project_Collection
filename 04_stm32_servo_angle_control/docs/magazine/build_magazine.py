# -*- coding: utf-8 -*-
"""Build the 5-page editorial-magazine HTML about project 04 (auto gate)."""

import base64
import io
import os
from pathlib import Path

from PIL import Image

ROOT = Path(r"D:\Users\Administrator\Desktop\单片机从0到100\04_stm32_servo_angle_control")
WORK = ROOT / "docs" / "magazine"
WORK.mkdir(parents=True, exist_ok=True)
ASSETS = WORK / "assets"
ASSETS.mkdir(exist_ok=True)

PAPER = (240, 238, 229)


def load(p):
    return Image.open(p).convert("RGB")


def save(im, name):
    out = ASSETS / name
    im.save(out, quality=90, optimize=True)
    return out


def crop(src, box, size=None):
    im = src.crop(box)
    if size:
        im = im.resize(size, Image.LANCZOS)
    return im


def data_uri(path):
    b = Path(path).read_bytes()
    return "data:image/png;base64," + base64.b64encode(b).decode()


demo = load(ROOT / "hardware" / "gate-demo.png")
ui = load(ROOT / "docs" / "ui-preview.png")
wiring = load(ROOT / "hardware" / "wiring.png")

left_panel = crop(demo, (60, 130, 780, 760))
right_panel = crop(demo, (820, 130, 1540, 760))
ir_zoom = crop(demo, (380, 560, 780, 760))

screen1 = crop(ui, (50, 55, 430, 300))
screen2 = crop(ui, (470, 55, 850, 300))
screen3 = crop(ui, (890, 55, 1270, 300))

# --- cover: portrait collage 704x1160 ---
from PIL import ImageOps

def fit_cover(im, size):
    return ImageOps.fit(im, size, Image.LANCZOS)

gate_clean = demo.crop((820, 215, 1540, 755))          # 去掉面板小标题
screen_clean = ui.crop((55, 90, 425, 320))             # 只取屏幕内容
ir_clean = demo.crop((395, 555, 780, 760))

def plate(im, width, top):
    h = round(im.height * width / im.width)
    im2 = im.resize((width, h), Image.LANCZOS)
    cover.paste(im2, ((704 - width) // 2, top))
    return top + h

cover = Image.new("RGB", (704, 1160), PAPER)
y = plate(gate_clean, 640, 40)
y = plate(screen_clean, 470, y + 46)
y = plate(wiring.crop((150, 640, 700, 800)), 600, y + 46)
save(cover, "cover.png")

save(screen1, "screen-auto.png")
save(screen2, "screen-wait.png")
save(screen3, "screen-run.png")
save(left_panel, "gate-closed.png")
save(right_panel, "gate-open.png")
save(ir_zoom, "ir-zoom.png")
save(crop(wiring, (110, 150, 1010, 820)), "wiring-top.png")
save(crop(wiring, (110, 940, 1600, 1220)), "wiring-power.png")
save(crop(wiring, (950, 150, 1600, 800)), "wiring-right.png")
save(crop(wiring, (100, 100, 1600, 1230)), "wiring-full.png")
save(crop(wiring, (150, 640, 700, 800)), "wiring-ir-zoom.png")

from PIL import ImageOps
def fit(im, size):
    return ImageOps.fit(im, size, Image.LANCZOS)

def triptych(images, name):
    canvas = Image.new("RGB", (1816, 400), PAPER)
    for i, im in enumerate(images):
        canvas.paste(fit(im, (600, 400)), (i * 608, 0))
    save(canvas, name)

screen1_clean = ui.crop((55, 92, 425, 318))
screen2_clean = ui.crop((475, 92, 845, 318))
save(screen1_clean, "screen-auto-clean.png")
save(screen2_clean, "screen-wait-clean.png")

gate_closed_clean = demo.crop((60, 215, 780, 755))
gate_open_clean = demo.crop((820, 215, 1540, 755))

def triptych_contain(images, name):
    canvas = Image.new("RGB", (1816, 400), PAPER)
    for i, im in enumerate(images):
        im2 = ImageOps.contain(im, (600, 400), Image.LANCZOS)
        canvas.paste(im2, (i * 608 + (600 - im2.width) // 2, (400 - im2.height) // 2))
    save(canvas, name)

triptych_contain([gate_closed_clean, screen1_clean, gate_open_clean], "triptych-auto.png")

concept = Image.new("RGB", (560, 980), PAPER)
cy = 70
for im in (screen1_clean, gate_closed_clean):
    im2 = ImageOps.contain(im, (500, 420), Image.LANCZOS)
    concept.paste(im2, ((560 - im2.width) // 2, cy))
    cy += im2.height + 60
save(concept, "concept-vertical.png")
triptych([wiring.crop((110, 150, 1010, 820)), wiring.crop((110, 940, 1600, 1220)),
          wiring.crop((950, 150, 1600, 800))], "triptych-wiring.png")

IMAGES = {
    "COVER": "cover.png",
    "SCREEN_AUTO": "screen-auto.png",
    "SCREEN_WAIT": "screen-wait.png",
    "SCREEN_RUN": "screen-run.png",
    "GATE_CLOSED": "gate-closed.png",
    "GATE_OPEN": "gate-open.png",
    "IR_ZOOM": "ir-zoom.png",
    "WIRING_TOP": "wiring-top.png",
    "WIRING_POWER": "wiring-power.png",
    "WIRING_RIGHT": "wiring-right.png",
    "WIRING_FULL": "wiring-full.png",
    "WIRING_IR": "wiring-ir-zoom.png",
    "TRIPTYCH_AUTO": "triptych-auto.png",
    "SCREEN_CLEAN": "screen-auto-clean.png",
    "CONCEPT": "concept-vertical.png",
    "SCREEN_WAIT_CLEAN": "screen-wait-clean.png",
    "TRIPTYCH_WIRING": "triptych-wiring.png",
}

# ---------------- HTML ----------------
HTML = r"""<!doctype html><html lang="zh-CN"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>自动闸门 · 项目 04 编辑专题</title><style>
:root{--paper:#F0EEE5;--ink:#27313B;--accent:#243E5C;--on-accent:#FFF8ED;--rule:4px solid var(--accent);--rule-soft:color-mix(in srgb,var(--accent) 25%,transparent);--collage-filter:none}
*{box-sizing:border-box}body{margin:0;background:#d4cdc3;color:var(--ink);font-family:"Noto Serif SC","Songti SC","Noto Serif CJK SC","SimSun",serif}
article[data-page]{width:1080px;height:1440px;margin:0 auto 32px;padding:62px;position:relative;background:var(--paper)}
h1,h2,h3,p,figure,blockquote{margin:0}img{display:block;max-width:100%;object-fit:contain}
p{font-size:23px;line-height:1.58;text-align:justify;word-break:normal;overflow-wrap:break-word}
p+p{margin-top:17px}h2{font-size:34px;line-height:1.25;font-weight:700}h3{font-size:27px;line-height:1.3;margin:25px 0 13px}
strong{font-weight:700}
.running{display:flex;justify-content:space-between;font:19px/1.4 "Noto Sans SC","PingFang SC",sans-serif;letter-spacing:2px;border-bottom:1px solid var(--accent);padding-bottom:12px;margin-bottom:27px;color:var(--accent)}
footer{position:absolute;bottom:32px;left:62px;right:62px;display:flex;justify-content:space-between;border-top:1px solid var(--rule-soft);padding-top:11px;font:17px/1.4 "Noto Sans SC","PingFang SC",sans-serif;color:var(--ink)}
.ruled{border-bottom:var(--rule);padding-bottom:9px;margin-bottom:16px}
.aside-caption{font:18px/1.45 "Noto Sans SC","PingFang SC",sans-serif;text-align:center;margin-top:13px;color:var(--accent)}
.editorial-callout{background:var(--accent);color:var(--on-accent);padding:32px;font-family:"Noto Sans SC","PingFang SC",sans-serif;font-weight:650}
.editorial-callout p{text-align:left;font-family:inherit}
.cover{background:var(--accent)!important;color:var(--on-accent)}
.cover .running{color:var(--on-accent);border-color:color-mix(in srgb,var(--on-accent) 36%,transparent)}
.cover .cover-art{position:absolute;width:704px;height:1160px;right:32px;bottom:88px;object-fit:contain}
.cover h1{position:absolute;top:300px;left:55px;display:flex;gap:15px;font-size:104px;line-height:1.42;font-weight:600}
.cover h1 span{writing-mode:vertical-rl;text-orientation:upright}
.cover h1 span:last-child{padding-top:120px;font-size:82px}
.cover .cover-deck{position:absolute;left:65px;top:140px;font-size:24px;line-height:1.65;letter-spacing:2px}
.cover footer{color:var(--on-accent);border-color:color-mix(in srgb,var(--on-accent) 36%,transparent)}
.three-column{display:grid;grid-template-columns:1fr .92fr 1fr;gap:30px;align-items:start}
.three-column .column>h2{font-size:31px;margin-bottom:18px}
.three-column h3:first-child{margin-top:0}
.three-column p{font-size:22px;line-height:1.52}
.central-spine .editorial-callout{padding:30px 27px}
.central-spine .editorial-callout p{font-size:35px;line-height:1.42;letter-spacing:2px}
.central-spine img{width:100%;height:490px;margin-top:26px}
.spine-tag{font:17px/1.5 "Noto Sans SC","PingFang SC",sans-serif;letter-spacing:3px;margin-bottom:26px;display:block}
.banner{background:var(--accent);color:var(--on-accent);padding:24px 34px;margin-bottom:22px}
.banner h1{font-size:62px;line-height:1.12;letter-spacing:1px}
.banner small{font:18px/1.4 "Noto Sans SC","PingFang SC",sans-serif;display:block;letter-spacing:4px;margin-bottom:12px}
.feature .intro{font-size:23px;line-height:1.5;margin-bottom:25px}
.feature h2{font-size:31px}
.picture-strip{margin:18px 0}
.picture-strip img{width:100%;height:196px;object-fit:contain}
.picture-strip figcaption{display:grid;grid-template-columns:repeat(3,1fr);gap:25px;margin-top:9px;font:18px/1.4 "Noto Sans SC","PingFang SC",sans-serif;text-align:center;color:var(--accent)}
.feature .section-copy{font-size:23px;line-height:1.55}
.feature .section-copy+h2{margin-top:26px}
.closing .intro{margin-bottom:28px}
.closing-grid{display:grid;grid-template-columns:285px 1fr;gap:32px}
.closing-grid h2{font-size:29px}
.closing-grid p{font-size:22px;line-height:1.55}
.narrow-column .sidebar-art{width:100%;height:300px;margin-top:20px}
.wide-column .picture-strip img{height:205px}
.wide-column .picture-strip{margin:24px 0}
.wide-column .picture-strip+h2{margin-top:26px}
.closing-callout{margin-top:24px;padding:27px 30px}
.closing-callout p{font-size:32px;line-height:1.4}
.closing-callout small{font:17px/1.4 "Noto Sans SC","PingFang SC",sans-serif;display:block;margin-bottom:13px;letter-spacing:2px}
.spec table{width:100%;border-collapse:collapse;margin:18px 0 22px;font:19px/1.45 "Noto Sans SC","PingFang SC",sans-serif}
.spec th,.spec td{border-bottom:1px solid var(--rule-soft);padding:7px 6px;text-align:left}
.spec th{color:var(--accent);letter-spacing:1px;border-bottom:2px solid var(--accent)}
.spec td:first-child{font-weight:700;width:130px}
.spec .wiring{width:100%;height:226px;margin:6px 0 6px;object-fit:contain}
.spec .cap{font:18px/1.45 "Noto Sans SC","PingFang SC",sans-serif;color:var(--accent);text-align:center;margin-bottom:0}
.collage{filter:var(--collage-filter)}
</style></head><body>

<!-- ============ 01 封面 ============ -->
<article data-page data-template="explainer-cover" class="cover" aria-label="封面：红外一响，栏杆抬起">
<div class="running"><span>项目手记 / 04 · 自动闸门</span><span>编辑专题</span></div>
<p class="cover-deck">红外测车，舵机抬杆<br>一个迷你道闸的拆解</p>
<img class="cover-art" src="__IMG_COVER__" alt="自动闸门实物效果：车靠近时抬杆">
<h1><span>红外一响</span><span>栏杆抬起</span></h1>
<footer><span>单片机从0到100 · 项目 04</span><span>01 / 05</span></footer></article>

<!-- ============ 02 三栏论述 ============ -->
<article data-page data-template="explainer-columns" class="columns-page" aria-label="论述：为什么道闸要分两种模式">
<div class="running"><span>自动闸门</span><span>论述 / 效率与决定权</span></div>
<div class="three-column">
<section class="column"><h2>车不等人，<br>人也得放心</h2>
<p>道闸有个天生的矛盾：车流要的是效率，现场要的是可控。全自动在无人值守时最省事，但有人值守的岗亭里，管理员往往希望“先看一眼，再决定放不放行”。这不是技术落后，而是场景不同。</p>
<p>所以这个项目把选择交回给人：编码器转一格，模式在<strong>自动</strong>和<strong>手动</strong>之间切换。同一套硬件、同一段固件，早上车流高峰可以无人值守，傍晚有活动时可以人工确认。</p>
<h3>自动模式：把判断交给探头</h3>
<p>红外避障模块装在闸机前方十几厘米，车头压过来，OUT 从高电平变成低电平。固件认定“有车”，立刻把舵机推到 90°，栏杆竖起。车开走、探头前空出来，再等 3 秒落杆——留出车尾通过的时间。</p>
<h3>同一套硬件，两种脾气</h3>
<p>红外、舵机、蜂鸣器一个没换，变的是判断顺序：自动模式“先执行，再报告”，手动模式“先报告，等指令”。一个模式变量，决定了同一辆车会遇到什么。</p>
</section>
<aside class="central-spine"><div class="editorial-callout"><span class="spine-tag">本页观点</span><p>自动解决<br>效率，<br>手动保留<br>决定权。</p></div>
<img class="concept-art" src="__IMG_CONCEPT__" alt="自动模式屏幕、红外特写与电源分配拼贴"><p class="aside-caption">有车、开闸、到位，屏幕四行说清当前状态</p></aside>
<section class="column"><h3>手动模式：把决定留给人</h3>
<p>切到手动，探头不再直接开闸，而是先“提醒”：蜂鸣器响 300 毫秒，屏幕上的状态开始闪「等待」。管理员看一眼情况，按一下触摸模块，栏杆才抬起；再按一下，落杆。</p>
<p>这样探头只负责“报告”，不负责“执行”。同一个红外信号，在不同模式下的含义完全不同——这正是模式切换存在的意义。</p>
<h3>消抖，比灵敏度更重要</h3>
<p>红外输出是开关量，树叶、飞虫、反光都会让它抖一下。固件要求电平稳定 50 毫秒才认，避免栏杆跟着抖动反复抬落。手动模式下更严格：只在“无车 → 有车”的上升沿提醒一次，不会一直响。</p>
<p>手动也不是“关掉自动”：探头仍在工作，只是它的输出从命令降级成提醒。切模式时如果探头前正好有车，蜂鸣器会立刻响一声——它在补报刚才没来得及说的那句话。</p>
<h3>经验是留给下一次的</h3>
<p>模式切换、消抖、延时，这三个动作都不复杂，却正好对应真实设备里最难处理的三件事：谁负责判断、信号可不可信、动作什么时候发生。</p>
</section></div><footer><span>自动闸门 · 项目 04</span><span>02 / 05</span></footer></article>

<!-- ============ 03 横向展开 ============ -->
<article data-page data-template="explainer-feature" class="feature" aria-label="展开：一次通行的三秒">
<div class="running"><span>自动闸门</span><span>展开 / 自动模式的一次通行</span></div>
<header class="banner"><small>从检测到落杆</small><h1>一次通行，<br>只有三秒</h1></header>
<p class="intro">车压到红外，抬杆；车走后三秒，落杆。这段流程里，固件要处理三件事：确认“真的有车”、决定“什么时候落杆”、控制“栏杆怎么动”。</p>
<h2 class="ruled">第一步，先确认真的有车</h2>
<figure class="picture-strip"><img src="__IMG_TRIPTYCH_AUTO__" alt="车靠近、屏幕显示有车、栏杆竖起三张并排"><figcaption><span>车压到探头，栏杆还在 0°</span><span>屏幕先变「有车」，再变「开闸」</span><span>90° 竖起，车通过后等待落杆</span></figcaption></figure>
<p class="section-copy">红外输出的只是一个高低电平，它分不清车和人。固件先做 50 毫秒消抖：电平连续稳定才承认变化。几行代码，决定了装置是“灵敏”还是“神经质”。</p>
<h2 class="ruled">第二步，什么时候落杆</h2>
<p class="section-copy">车走就落杆听着合理，实际很危险：车尾还没过完，栏杆已经压下来。固件用 3 秒延时补偿：探头前空出来开始计时，期间再来车就取消倒计时。</p>
<p class="section-copy">这 3 秒写在 <code>board.h</code> 的 <code>APP_AUTO_CLOSE_MS</code>，车长就调大，演示可以调到 1 秒。</p>
<h2 class="ruled">第三步，栏杆怎么动</h2>
<p class="section-copy">舵机要的是 50Hz 方波：0.5ms 对应 0°，2.5ms 对应 180°，中间线性映射。固件把角度换算成计数器值直接写进寄存器，剩下的交给 TIM2。</p>
<p class="section-copy">目标角度可以一步跳变，输出不会：主循环每 20ms 只走 2°，90° 要约 0.9 秒。栏杆是抬起来的，不是弹起来的。</p>
<footer><span>自动闸门 · 项目 04</span><span>03 / 05</span></footer></article>

<!-- ============ 04 不对称收束 ============ -->
<article data-page data-template="explainer-closing" class="closing" aria-label="收束：手动模式与安全兜底">
<div class="running"><span>自动闸门</span><span>收束 / 人、规则与兜底</span></div>
<h2 class="ruled">手动不是落后，是把决定权留给人</h2>
<p class="intro">自动解决“没人看着也能跑”，手动解决“有人看着更放心”。两者共用一套执行机构，差别只在谁按下最后那一下。</p>
<div class="closing-grid"><aside class="narrow-column"><h2 class="ruled">三个动作，<br>一条兜底</h2>
<p>短按：开闸；再短按：落杆。长按：不管什么状态，立刻强制落杆。</p>
<p>强制落杆放在长按上，是刻意的：不该被误触，需要时必须立刻生效。</p>
<img class="sidebar-art" src="__IMG_SCREEN_WAIT_CLEAN__" alt="手动模式屏幕：红外有车、状态等待"></aside>
<section class="wide-column"><h2 class="ruled">把“紧急”留给一个不容易误触的动作</h2>
<p>真实道闸都有急停和手动释放。这个迷你版保留了同样的思路：正常操作走短按，紧急动作走长按，800 毫秒才触发，不容易被路过的人碰到。</p>
<p>手动模式检测到车时，屏幕会闪「等待」、蜂鸣器响一声——不是催你，是提醒“探头看见车了”。</p>
<figure class="picture-strip"><img src="__IMG_WIRING_IR__" alt="红外避障模块 OUT 接 PA4 的接线特写"><p class="aside-caption">红外 OUT 接 PA4，检测到车输出低电平</p></figure>
<h2 class="ruled">一条可以复用的规则</h2>
<p>把“感知”和“执行”分开，是这个项目最值得带走的一条经验：红外只负责报告，模式决定谁来拍板。以后加超声波、加第二个探头、接远程开关，都只是在这条链路上加环节。</p>
<div class="editorial-callout closing-callout"><small>编辑结语 · 非人物引语</small><p>自动让人省心，<br>手动让人安心；<br>能随时切换，才是好用。</p></div>
</section></div><footer><span>自动闸门 · 项目 04</span><span>04 / 05</span></footer></article>

<!-- ============ 05 参数与实现 ============ -->
<article data-page data-template="explainer-feature" class="feature spec" aria-label="附录：25KB 里的取舍">
<div class="running"><span>自动闸门</span><span>附录 / 引脚与参数</span></div>
<header class="banner"><small>实现手记</small><h1>把逻辑<br>装进 25KB</h1></header>
<p class="intro">整套固件 25.3KB，不到 64KB Flash 的四成。空间不是问题，取舍在于哪些交给硬件定时器，哪些留给软件循环。</p>
<h2 class="ruled">引脚分工</h2>
<table><tr><th>引脚</th><th>用途</th><th>说明</th></tr>
<tr><td>PA0</td><td>舵机 PWM</td><td>TIM2_CH1，50Hz，0.5~2.5ms 对应 0~180°</td></tr>
<tr><td>PA1 / PA2</td><td>编码器 A / B</td><td>内部上拉，转一格切换自动/手动</td></tr>
<tr><td>PA3</td><td>触摸 OUT</td><td>手动模式开/关闸，长按强制落杆</td></tr>
<tr><td>PA4</td><td>红外 OUT</td><td>检测到车为低电平，50ms 消抖</td></tr>
<tr><td>PB5</td><td>蜂鸣器 IN</td><td>低电平触发，到位/提醒音</td></tr>
<tr><td>PB10 / PB11</td><td>OLED SCL / SDA</td><td>I2C2，四行状态界面</td></tr></table>
<h2 class="ruled">PWM 与缓动</h2>
<p class="section-copy">舵机要 50Hz 方波，脉宽决定角度：<code>CCR = 500 + angle × 2000 / 180</code>（µs）。TIM2 用 72 分频得到 1MHz 计数，周期 20000 就是 20ms。</p>
<p class="section-copy">目标角度可以一步跳变，输出不会：每 20ms 只走 2°，90° 约 0.9 秒。栏杆是抬起来的，不是弹起来的。</p>
<img class="wiring" src="__IMG_WIRING_FULL__" alt="项目 04 完整接线图"><p class="cap">完整接线：舵机单独 5V，其余模块 3V3，GND 全部共地</p>
<footer><span>自动闸门 · 项目 04</span><span>05 / 05</span></footer></article>

</body></html>
"""

for token, name in IMAGES.items():
    HTML = HTML.replace("__IMG_" + token + "__", data_uri(ASSETS / name))

out = WORK / "magazine.html"
out.write_text(HTML, encoding="utf-8")
print("written:", out, out.stat().st_size // 1024, "KB")
