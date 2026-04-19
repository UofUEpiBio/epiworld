(function () {
  "use strict";

  const HERO_SELECTOR = ".epiworld-network-hero";
  const CANVAS_SELECTOR = ".epiworld-network-canvas";

  const config = {
    particleCount: 100,
    connectionDistance: 150,
    speed: 0.2,
    particleColor: "rgba(255, 255, 255, 0.2)",
    lineOpacityScale: 0.1
  };

  let animationFrameId = null;
  let resizeObserver = null;
  let cleanupCurrent = null;

  function prefersReducedMotion() {
    return window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  }

  function startHeroAnimation() {
    if (cleanupCurrent) {
      cleanupCurrent();
      cleanupCurrent = null;
    }

    const hero = document.querySelector(HERO_SELECTOR);
    if (!hero) {
      return;
    }

    const canvas = hero.querySelector(CANVAS_SELECTOR);
    if (!canvas) {
      return;
    }

    const ctx = canvas.getContext("2d");
    if (!ctx) {
      return;
    }

    let particles = [];

    function resizeCanvas() {
      const ratio = Math.min(window.devicePixelRatio || 1, 2);
      const width = hero.clientWidth;
      const height = hero.clientHeight;

      canvas.width = Math.floor(width * ratio);
      canvas.height = Math.floor(height * ratio);
      canvas.style.width = width + "px";
      canvas.style.height = height + "px";

      ctx.setTransform(ratio, 0, 0, ratio, 0, 0);
    }

    class Particle {
      constructor() {
        this.x = Math.random() * hero.clientWidth;
        this.y = Math.random() * hero.clientHeight;
        this.vx = (Math.random() - 0.5) * config.speed * 2;
        this.vy = (Math.random() - 0.5) * config.speed * 2;
        this.radius = Math.random() * 2 + 1;
      }

      update() {
        this.x += this.vx;
        this.y += this.vy;

        if (this.x < 0 || this.x > hero.clientWidth) {
          this.vx = -this.vx;
        }

        if (this.y < 0 || this.y > hero.clientHeight) {
          this.vy = -this.vy;
        }
      }

      draw() {
        ctx.beginPath();
        ctx.arc(this.x, this.y, this.radius, 0, Math.PI * 2);
        ctx.fillStyle = config.particleColor;
        ctx.fill();
      }
    }

    function initParticles() {
      particles = [];
      for (let i = 0; i < config.particleCount; i += 1) {
        particles.push(new Particle());
      }
    }

    function drawConnections() {
      for (let i = 0; i < particles.length; i += 1) {
        for (let j = i + 1; j < particles.length; j += 1) {
          const dx = particles[i].x - particles[j].x;
          const dy = particles[i].y - particles[j].y;
          const distance = Math.hypot(dx, dy);

          if (distance < config.connectionDistance) {
            const opacity = 1 - distance / config.connectionDistance;
            ctx.beginPath();
            ctx.moveTo(particles[i].x, particles[i].y);
            ctx.lineTo(particles[j].x, particles[j].y);
            ctx.strokeStyle = "rgba(255, 255, 255, " + opacity * config.lineOpacityScale + ")";
            ctx.lineWidth = 1;
            ctx.stroke();
          }
        }
      }
    }

    function animate() {
      ctx.clearRect(0, 0, hero.clientWidth, hero.clientHeight);

      for (let i = 0; i < particles.length; i += 1) {
        particles[i].update();
        particles[i].draw();
      }

      drawConnections();
      animationFrameId = window.requestAnimationFrame(animate);
    }

    resizeCanvas();
    initParticles();

    if (!prefersReducedMotion()) {
      animate();
    } else {
      for (let i = 0; i < particles.length; i += 1) {
        particles[i].draw();
      }
      drawConnections();
    }

    var onResize = function () {
      resizeCanvas();
      initParticles();
    };

    window.addEventListener("resize", onResize);

    if (typeof ResizeObserver !== "undefined") {
      resizeObserver = new ResizeObserver(onResize);
      resizeObserver.observe(hero);
    }

    cleanupCurrent = function () {
      window.removeEventListener("resize", onResize);

      if (resizeObserver) {
        resizeObserver.disconnect();
        resizeObserver = null;
      }

      if (animationFrameId) {
        window.cancelAnimationFrame(animationFrameId);
        animationFrameId = null;
      }
    };
  }

  if (typeof document$ !== "undefined" && document$.subscribe) {
    document$.subscribe(startHeroAnimation);
  } else {
    document.addEventListener("DOMContentLoaded", startHeroAnimation);
  }
})();
