document.getElementById("greet").addEventListener("click", async () => {
  const out = document.getElementById("out");
  try {
    const res = await window.invoke("greet", { name: document.getElementById("name").value });
    out.textContent = res.error ? "error: " + res.error : res.message;
  } catch (e) {
    out.textContent = "error: " + e;
  }
});
