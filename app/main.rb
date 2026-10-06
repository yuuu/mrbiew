App.on("greet") { |args| { message: "Hello, #{args && args["name"] || "world"}" } }
