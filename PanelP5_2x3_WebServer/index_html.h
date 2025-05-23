const char webpage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>PT. WAPRASTA GLOBAL TEKNOLOGI</title>
    <style>
      body {
        background-color: black;
        color: white;
        font-family: Arial, sans-serif;
        text-align: center;
        margin: 0;
        padding: 0;
      }
      h1 {
        margin-top: 20px;
        margin-bottom: 0px;
        margin-left: auto;
        margin-right: auto;
        font-size: 25px;
        text-align: center;
        color: rgb(0, 0, 0);
      }
      h2 {
        margin-top: 0px;
        margin-bottom: 0px;
        margin-left: auto;
        margin-right: auto;
        font-size: 25px;
        text-align: center;
        color: rgb(0, 0, 0);
      }
      form {
        display: inline-block;
        margin-top: 100px;
        text-align: center;
        font-size: 16px;
        width: 535px;
        background-color: #50cf54;
        border-radius: 5px;
      }
      label {
        margin-top: 10px;
        display: inline-block;
        width: 50px;
        text-align: left;
        color: rgb(0, 0, 0);
        font-weight: bold;
      }
      label2 {
        margin-top: 10px;
        display: inline-block;
        width: 5px;
        text-align: center;
        color: rgb(0, 0, 0);
        font-weight: bold;
        
      }
      input[type="text"] {
        margin-top: 10px;
        width: calc(100% - 150px);
        padding: 10px;
        font-size: 16px;
        border-radius: 5px;
        border-color: chocolate;
        background-color: #ffffff;
      }
      input[type="submit"] {
        margin-top: 20px;
        margin-bottom: 20px;
        padding: 10px 20px;
        font-size: 16px;
        background-color: #155c18;
        color: white;
        border: none;
        cursor: pointer;
        border-radius: 5px;
        font-weight: bold;
      }
      input[type="submit"]:hover {
        background-color: #45a049;
      }
      .checkbox-container {
        margin: 20px 0;
        text-align: center;
      }
      .checkbox-container input[type="checkbox"] {
        transform: scale(1.5);
        margin-top: 2;
        margin-right: 0px;
        vertical-align: middle;
      }
      .checkbox-container label {
        display: inline;
        margin-top: auto;
        margin-left: 10px;
        margin-right: 10px;
        font-size: 16px;
        cursor: pointer;
      }
      .hidden {
        display: none;
      }
    </style>
    <script>
      function updateForm() {
        const checkbox1 = document.getElementById('checkbox1');
        const checkbox4 = document.getElementById('checkbox4');
        if (checkbox1.checked && checkbox4.checked) {
            document.getElementById('code').classList.add('hidden');
            document.getElementById('text').classList.add('hidden');
            document.getElementById('text2').classList.add('hidden');
            document.getElementById('text3').classList.add('hidden');
            document.getElementById('text4').classList.add('hidden');
        } else if (checkbox1.checked) {
            document.getElementById('code').classList.remove('hidden');
            document.getElementById('text').classList.remove('hidden');
            document.getElementById('text2').classList.add('hidden');
            document.getElementById('text3').classList.add('hidden');
            document.getElementById('text4').classList.add('hidden');
        } else if (checkbox4.checked) {
            document.getElementById('code').classList.remove('hidden');
            document.getElementById('text').classList.remove('hidden');
            document.getElementById('text2').classList.remove('hidden');
            document.getElementById('text3').classList.remove('hidden');
            document.getElementById('text4').classList.remove('hidden');
        } else {
            document.getElementById('code').classList.add('hidden');
            document.getElementById('text').classList.add('hidden');
            document.getElementById('text2').classList.add('hidden');
            document.getElementById('text3').classList.add('hidden');
            document.getElementById('text4').classList.add('hidden');
        }
      }
    </script>
  </head>
  <body>

    <form action="/submit" method="get">
      <h1>Display1_PT. WAPRASTA GLOBAL TEKNOLOGI</h1>
      <h2>------------------------------------------------------</h2>
      <div class="checkbox-container">
        <label for="checkbox1">1</label>
        <input type="checkbox" id="checkbox1" name="option" value="1" onclick="updateForm();">
        <label for="checkbox4">4</label>
        <input type="checkbox" id="checkbox4" name="option" value="4" onclick="updateForm();">
      </div>

      <div id="code" class="hidden">
      <label for="code">Kode</label>
      <label2 for="code">:</label2>
      <input type="text" id="code" name="code"><br>
      </div>

      <div id="text" class="hidden">
      <label for="text">Teks 1</label>
      <label2 for="text">:</label2>
      <input type="text" id="text" name="text1"><br>
      </div>

      <div id="text2" class="hidden">
        <label for="text2">Teks 2</label>
        <label2 for="text2">:</label2>
        <input type="text" id="text2" name="text2"><br>
      </div>

      <div id="text3" class="hidden">
        <label for="text3">Teks 3</label>
        <label2 for="text3">:</label2>
        <input type="text" id="text3" name="text3"><br>
      </div>
      
      <div id="text4" class="hidden">
        <label for="text4">Teks 4</label>
        <label2 for="text4">:</label2>
        <input type="text" id="text4" name="text4"><br>
      </div>
      <input type="submit" value="Kirim">
    </form>
  </body>
</html>
)rawliteral";

