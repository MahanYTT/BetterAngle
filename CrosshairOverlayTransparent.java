import javax.swing.*;
import javax.swing.event.ChangeListener;
import java.awt.*;
import java.awt.event.*;
import java.awt.geom.Line2D;
import java.util.ArrayList;
import java.io.*;
import java.nio.file.*;

public class CrosshairOverlayTransparent extends JWindow {
	private double a = 0;
	private boolean v = true, pulseActive = false;
	private boolean everythingVisible = true;
	private javax.swing.Timer pulseTimer;
	private float pulseT = 0f;
	private double offX = 0.0, offY = 0.0;
	private float lineThickness = 1.0f;
	private Color crosshairColor = Color.RED;
	private Point dS, wD;
	private JFrame f;
	private JButton bH;
	private JLabel lCoords;
	private ArrayList<P> s = new ArrayList<>();
	private JPanel sP;
	private static final String SAVE_FILE = "crosshair_positions.dat";
	private static final String LAST_STATE_FILE = "last_state.dat";

	private boolean keyN = false, keyP = false;

	class P implements Serializable {
		private static final long serialVersionUID = 1L;
		double x, y, a;
		String n;
		P(double x, double y, double a, String n) {
			this.x = x;
			this.y = y;
			this.a = a;
			this.n = n;
		}
	}

	public CrosshairOverlayTransparent() {
		loadPositions();
		loadLastState();
		createControl();
		updateCoords();
		GraphicsConfiguration g = GraphicsEnvironment
			.getLocalGraphicsEnvironment()
			.getDefaultScreenDevice().getDefaultConfiguration();
		Rectangle b = g.getBounds();
		Insets i = Toolkit.getDefaultToolkit().getScreenInsets(g);
		setBounds(b.x + i.left, b.y + i.top,
			b.width - i.left - i.right,
			b.height - i.top - i.bottom);
		setAlwaysOnTop(true);
		setBackground(new Color(0, 0, 0, 0));
		setFocusableWindowState(false);
		JPanel p = new JPanel(null) {
			protected void paintComponent(Graphics g) {
				super.paintComponent(g);
				if (v && everythingVisible) {
					drawCross((Graphics2D) g);
				}
			}
		};
		p.setOpaque(false);
		setContentPane(p);
		setVisible(true);
		registerGlobalHotkey();
	}

	private void registerGlobalHotkey() {
		try {
			Class<?> gmClass   = Class.forName("com.github.kwhat.jnativehook.GlobalScreen");
			Class<?> logClass  = Class.forName("java.util.logging.Logger");
			Class<?> lvlClass  = Class.forName("java.util.logging.Level");
			// Silence JNativeHook logger
			Object logger = logClass.getMethod("getLogger", String.class).invoke(null, "com.github.kwhat.jnativehook");
			Object offLevel = lvlClass.getField("OFF").get(null);
			logClass.getMethod("setLevel", lvlClass).invoke(logger, offLevel);

			gmClass.getMethod("registerNativeHook").invoke(null);

			// Build listener via proxy
			Class<?> listenerIface = Class.forName("com.github.kwhat.jnativehook.keyboard.NativeKeyListener");
			Class<?> eventClass    = Class.forName("com.github.kwhat.jnativehook.keyboard.NativeKeyEvent");

			java.lang.reflect.InvocationHandler handler = (proxy, method, args) -> {
				if (method.getName().equals("nativeKeyPressed") && args != null && args.length > 0) {
					int vc = (int) eventClass.getMethod("getKeyCode").invoke(args[0]);
					// VC_U = 22, VC_8 = 9 in JNativeHook key codes
					if (vc == 22) keyN = true;
					if (vc == 9)  keyP = true;
					if (keyN && keyP) {
						keyN = false; keyP = false;
						SwingUtilities.invokeLater(this::toggleEverything);
					}
				}
				if (method.getName().equals("nativeKeyReleased") && args != null && args.length > 0) {
					int vc = (int) eventClass.getMethod("getKeyCode").invoke(args[0]);
					if (vc == 22) keyN = false;
					if (vc == 9)  keyP = false;
				}
				return null;
			};
			Object proxy = java.lang.reflect.Proxy.newProxyInstance(
				listenerIface.getClassLoader(), new Class<?>[]{ listenerIface }, handler);
			gmClass.getMethod("addNativeKeyListener", listenerIface).invoke(null, proxy);
			System.out.println("Global hotkey active: U + 8 to show/hide");
		} catch (Exception ex) {
			// JNativeHook not found — fall back to AWT-only (app must be focused)
			System.out.println("JNativeHook not found. Using AWT hotkey (app must be focused). Hotkey: U + 8");
			KeyboardFocusManager.getCurrentKeyboardFocusManager()
				.addKeyEventDispatcher(e -> {
					int code = e.getKeyCode();
					if (e.getID() == KeyEvent.KEY_PRESSED) {
						if (code == KeyEvent.VK_U) keyN = true;
						if (code == KeyEvent.VK_8) keyP = true;
						if (keyN && keyP) {
							toggleEverything();
							keyN = false; keyP = false;
						}
					}
					if (e.getID() == KeyEvent.KEY_RELEASED) {
						if (code == KeyEvent.VK_U) keyN = false;
						if (code == KeyEvent.VK_8) keyP = false;
					}
					return false;
				});
		}
	}

	private void toggleEverything() {
		everythingVisible = !everythingVisible;
		f.setVisible(everythingVisible);
		repaint();
	}

	private void loadPositions() {
		File file = new File(SAVE_FILE);
		if (file.exists()) {
			try (ObjectInputStream ois = new ObjectInputStream(
				new FileInputStream(file))) {
				s = (ArrayList<P>) ois.readObject();
			} catch (Exception e) { initDefaultPositions(); }
		} else { initDefaultPositions(); }
	}

	private void initDefaultPositions() {
		s.clear();
		s.add(new P(0.0, 0.0, 0.0, "🎯 Screen Center"));
		s.add(new P(0.0, 24.5, 0.0, "testing"));
	}

	private void savePositions() {
		try (ObjectOutputStream oos = new ObjectOutputStream(
			new FileOutputStream(SAVE_FILE))) {
			oos.writeObject(s);
		} catch (Exception e) {}
	}

	private void saveLastState() {
		try (DataOutputStream dos = new DataOutputStream(
			new FileOutputStream(LAST_STATE_FILE))) {
			dos.writeDouble(offX);
			dos.writeDouble(offY);
			dos.writeDouble(a);
			dos.writeFloat(lineThickness);
			dos.writeInt(crosshairColor.getRGB());
		} catch (Exception e) {}
	}

	private void loadLastState() {
		File file = new File(LAST_STATE_FILE);
		if (!file.exists()) return;
		try (DataInputStream dis = new DataInputStream(
			new FileInputStream(file))) {
			offX = dis.readDouble();
			offY = dis.readDouble();
			dis.readDouble(); // angle ignored — always 0
			a = 0;
			if (dis.available() >= 4) lineThickness = dis.readFloat();
			if (dis.available() >= 4) crosshairColor = new Color(dis.readInt(), true);
		} catch (Exception e) {}
	}

	private void createControl() {
		f = new JFrame("Crosshair Control");
		f.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		f.setAlwaysOnTop(true);
		f.addWindowListener(new WindowAdapter() {
			public void windowClosing(WindowEvent e) { savePositions(); saveLastState(); }
		});
		JPanel mainPanel = new JPanel(new BorderLayout(10, 10)) {
			protected void paintComponent(Graphics g) {
				Graphics2D g2 = (Graphics2D) g;
				g2.setRenderingHint(RenderingHints.KEY_RENDERING,
					RenderingHints.VALUE_RENDER_QUALITY);
				int w = getWidth(), h = getHeight();
				GradientPaint gp = new GradientPaint(0,0,
					new Color(45,45,68), 0,h, new Color(28,28,45));
				g2.setPaint(gp); g2.fillRect(0,0,w,h);
			}
		};
		mainPanel.setBorder(BorderFactory.createEmptyBorder(15,15,15,15));
		mainPanel.setOpaque(true);

		lCoords = new JLabel("X: 0.0  Y: 0.0  Angle: 0°", SwingConstants.CENTER);
		lCoords.setFont(new Font("Arial", Font.PLAIN, 14));
		lCoords.setForeground(new Color(150,200,255));

		JPanel topPanel = new JPanel(new BorderLayout(5,5));
		topPanel.setOpaque(false);
		topPanel.add(lCoords, BorderLayout.CENTER);

		// 2x2 button grid — no fake placeholder button
		JPanel btnPanel = new JPanel(new GridLayout(2,2,10,10));
		btnPanel.setOpaque(false);
		bH = mkBtn("Hide", new Color(244,67,54));
		bH.addActionListener(e -> {
			v = !v; bH.setText(v?"Hide":"Show");
			bH.setBackground(v?new Color(244,67,54):new Color(76,175,80));
			repaint();
		});
		JButton bReset = mkBtn("Reset Center", new Color(103,58,183));
		bReset.addActionListener(e -> { offX=0.0; offY=0.0; updateCoords(); repaint(); });
		JButton bSave = mkBtn("Save Position", new Color(33,150,243));
		bSave.addActionListener(e -> {
			String name = JOptionPane.showInputDialog(f,
				"Position name:","Save Position", JOptionPane.PLAIN_MESSAGE);
			if (name!=null && !name.trim().isEmpty()) {
				s.add(new P(offX,offY,a,name.trim()));
				savePositions(); updateSavedList();
			}
		});
		JButton bPulse = mkBtn("Pulse", new Color(156,39,176));
		bPulse.addActionListener(e -> {
			pulseActive = !pulseActive;
			if (pulseActive) {
				bPulse.setText("Stop Pulse");
				bPulse.setBackground(new Color(211,47,47));
				pulseT = 0f;
				pulseTimer = new javax.swing.Timer(16, evt -> {
					pulseT += 0.08f;
					float opacity = 0.5f + 0.5f * (float)Math.sin(pulseT);
					setOpacity(Math.max(0.05f, opacity));
				});
				pulseTimer.start();
			} else {
				bPulse.setText("Pulse");
				bPulse.setBackground(new Color(156,39,176));
				if (pulseTimer != null) pulseTimer.stop();
				setOpacity(1.0f);
			}
		});
		btnPanel.add(bH); btnPanel.add(bReset);
		btnPanel.add(bSave); btnPanel.add(bPulse);

		// --- Colour picker ---
		JPanel colorPanel = new JPanel(new BorderLayout(5,4));
		colorPanel.setOpaque(false);
		JLabel colorLabel = new JLabel("Crosshair colour:", SwingConstants.CENTER);
		colorLabel.setForeground(Color.WHITE);
		colorLabel.setFont(new Font("Arial", Font.PLAIN, 12));

		JPanel colorRow = new JPanel(new FlowLayout(FlowLayout.CENTER, 5, 0));
		colorRow.setOpaque(false);

		JButton colorSwatch = new JButton();
		colorSwatch.setPreferredSize(new Dimension(36, 26));
		colorSwatch.setBackground(crosshairColor);
		colorSwatch.setFocusPainted(false);
		colorSwatch.setCursor(new Cursor(Cursor.HAND_CURSOR));
		colorSwatch.setToolTipText("Click to pick colour");

		JTextField hexField = new JTextField(colorToHex(crosshairColor), 7);
		hexField.setBackground(new Color(55,55,75));
		hexField.setForeground(Color.WHITE);
		hexField.setCaretColor(Color.WHITE);
		hexField.setFont(new Font("Monospaced", Font.PLAIN, 12));
		hexField.setBorder(BorderFactory.createCompoundBorder(
			BorderFactory.createLineBorder(new Color(100,100,120),1,true),
			BorderFactory.createEmptyBorder(2,4,2,4)));

		colorSwatch.addActionListener(e -> showSpectrumPicker(colorSwatch, hexField));
		hexField.addActionListener(e -> applyHex(hexField, colorSwatch));
		hexField.addFocusListener(new FocusAdapter() {
			public void focusLost(FocusEvent e) { applyHex(hexField, colorSwatch); }
		});

		colorRow.add(colorSwatch);
		colorRow.add(hexField);

		colorPanel.add(colorLabel, BorderLayout.NORTH);
		colorPanel.add(colorRow, BorderLayout.CENTER);

		// --- Fine move ---
		JPanel finePanel = new JPanel(new GridLayout(2,2,5,5));
		finePanel.setOpaque(false);
		JButton bXMinus = mkBtn("X -0.5", new Color(70,70,90));
		bXMinus.addActionListener(e -> { offX-=0.5; updateCoords(); repaint(); });
		JButton bXPlus = mkBtn("X +0.5", new Color(70,70,90));
		bXPlus.addActionListener(e -> { offX+=0.5; updateCoords(); repaint(); });
		JButton bYMinus = mkBtn("Y -0.5", new Color(70,70,90));
		bYMinus.addActionListener(e -> { offY-=0.5; updateCoords(); repaint(); });
		JButton bYPlus = mkBtn("Y +0.5", new Color(70,70,90));
		bYPlus.addActionListener(e -> { offY+=0.5; updateCoords(); repaint(); });
		finePanel.add(bXMinus); finePanel.add(bXPlus);
		finePanel.add(bYMinus); finePanel.add(bYPlus);

		JLabel hotkeyHint = new JLabel("press U + 8 to hide/show crosshair (global)");
		hotkeyHint.setForeground(new Color(110,110,140));
		hotkeyHint.setFont(new Font("Arial", Font.ITALIC, 10));
		hotkeyHint.setHorizontalAlignment(SwingConstants.CENTER);

		JPanel fineContainer = new JPanel(new BorderLayout(5,4));
		fineContainer.setOpaque(false);
		JLabel fineLabel = new JLabel("Fine move (0.5 px):");
		fineLabel.setForeground(Color.WHITE);
		fineContainer.add(fineLabel, BorderLayout.NORTH);
		fineContainer.add(finePanel, BorderLayout.CENTER);
		fineContainer.add(hotkeyHint, BorderLayout.SOUTH);

		// --- Thickness slider ---
		JPanel thickPanel = new JPanel(new BorderLayout(5,5));
		thickPanel.setOpaque(false);
		JLabel thickLabel = new JLabel(String.format("Line thickness: %.1f px", lineThickness));
		thickLabel.setForeground(Color.WHITE);
		thickLabel.setFont(new Font("Arial", Font.PLAIN, 12));
		int sliderInit = Math.round(lineThickness * 10);
		JSlider thickSlider = new JSlider(1, 100, sliderInit);
		thickSlider.setOpaque(false);
		thickSlider.addChangeListener(e -> {
			lineThickness = thickSlider.getValue() / 10.0f;
			thickLabel.setText(String.format("Line thickness: %.1f px", lineThickness));
			repaint();
		});
		thickPanel.add(thickLabel, BorderLayout.NORTH);
		thickPanel.add(thickSlider, BorderLayout.CENTER);

		// Saved positions — dynamic, no scroll, grows with entries
		sP = new JPanel();
		sP.setLayout(new BoxLayout(sP, BoxLayout.Y_AXIS));
		sP.setOpaque(false);
		updateSavedList();
		JPanel savedWrapper = new JPanel(new BorderLayout());
		savedWrapper.setOpaque(false);
		savedWrapper.setBorder(BorderFactory.createTitledBorder(
			BorderFactory.createLineBorder(new Color(100,100,120),1,true),
			"Saved Positions",0,0,
			new Font("Arial",Font.BOLD,12),Color.WHITE));
		savedWrapper.add(sP, BorderLayout.CENTER);

		// Assemble
		JPanel bottomControls = new JPanel(new BorderLayout(5,8));
		bottomControls.setOpaque(false);
		bottomControls.add(colorPanel, BorderLayout.NORTH);
		bottomControls.add(fineContainer, BorderLayout.CENTER);
		bottomControls.add(thickPanel, BorderLayout.SOUTH);

		JPanel midPanel = new JPanel(new BorderLayout(10,10));
		midPanel.setOpaque(false);
		midPanel.add(btnPanel, BorderLayout.NORTH);
		midPanel.add(bottomControls, BorderLayout.SOUTH);

		JPanel centerPanel = new JPanel(new BorderLayout(10,10));
		centerPanel.setOpaque(false);
		centerPanel.add(midPanel, BorderLayout.CENTER);
		centerPanel.add(savedWrapper, BorderLayout.SOUTH);

		mainPanel.add(topPanel, BorderLayout.NORTH);
		mainPanel.add(centerPanel, BorderLayout.CENTER);
		f.add(mainPanel); f.pack();
		f.setLocationRelativeTo(null); f.setVisible(true);
	}

	private String colorToHex(Color c) {
		return String.format("#%02X%02X%02X", c.getRed(), c.getGreen(), c.getBlue());
	}

	private void applyHex(JTextField hexField, JButton swatch) {
		try {
			String txt = hexField.getText().trim();
			if (!txt.startsWith("#")) txt = "#" + txt;
			Color c = Color.decode(txt);
			crosshairColor = c;
			swatch.setBackground(c);
			hexField.setText(colorToHex(c));
			repaint();
		} catch (NumberFormatException ex) {
			hexField.setText(colorToHex(crosshairColor));
		}
	}

	private void updateCoords() {
		if (lCoords != null)
			lCoords.setText(String.format("X: %.1f  Y: %.1f  Angle: %d°", offX, offY, (int)a));
		repaint();
	}

	private void updateSavedList() {
		sP.removeAll();
		for (int i = 0; i < s.size(); i++) {
			P p = s.get(i);
			JPanel row = new JPanel(new BorderLayout(5,0));
			row.setOpaque(false);
			row.setMaximumSize(new Dimension(Short.MAX_VALUE,35));
			String text = String.format("%s (%.1f, %.1f, %d°)",
				p.n, p.x, p.y, (int)p.a);
			JButton load = new JButton(text);
			load.setFont(new Font("Arial",Font.PLAIN,11));
			load.setBackground(new Color(63,81,181));
			load.setForeground(Color.WHITE);
			load.setFocusPainted(false);
			load.setBorderPainted(false);
			load.setCursor(new Cursor(Cursor.HAND_CURSOR));
			int idx = i;
			load.addActionListener(e -> {
				P sp = s.get(idx);
				offX=sp.x; offY=sp.y; a=0;
				updateCoords(); repaint();
			});
			row.add(load, BorderLayout.CENTER);
			JPanel bp = new JPanel(new FlowLayout(FlowLayout.RIGHT,2,0));
			bp.setOpaque(false);
			JButton edit = new JButton("✏");
			edit.setFont(new Font("Arial",Font.BOLD,14));
			edit.setPreferredSize(new Dimension(30,30));
			edit.setBackground(new Color(255,152,0));
			edit.setForeground(Color.WHITE);
			edit.setFocusPainted(false); edit.setBorderPainted(false);
			edit.setCursor(new Cursor(Cursor.HAND_CURSOR));
			edit.addActionListener(e -> { editPosition(idx); });
			bp.add(edit);
			if (!p.n.equals("🎯 Screen Center")) {
				JButton del = new JButton("×");
				del.setFont(new Font("Arial",Font.BOLD,16));
				del.setPreferredSize(new Dimension(30,30));
				del.setBackground(new Color(244,67,54));
				del.setForeground(Color.WHITE);
				del.setFocusPainted(false);
				del.setBorderPainted(false);
				del.setCursor(new Cursor(Cursor.HAND_CURSOR));
				del.addActionListener(e -> {
					s.remove(idx); savePositions(); updateSavedList();
				});
				bp.add(del);
			}
			row.add(bp, BorderLayout.EAST);
			sP.add(row);
			sP.add(Box.createVerticalStrut(5));
		}
		sP.revalidate(); sP.repaint();
		if (f != null) { f.pack(); }
	}

	private void editPosition(int idx) {
		P p = s.get(idx);
		JPanel panel = new JPanel(new GridLayout(4,2,5,5));
		panel.setBorder(BorderFactory.createEmptyBorder(10,10,10,10));
		JTextField tfName = new JTextField(p.n);
		JTextField tfX = new JTextField(String.valueOf(p.x));
		JTextField tfY = new JTextField(String.valueOf(p.y));
		JTextField tfAngle = new JTextField(String.valueOf((int)p.a));
		panel.add(new JLabel("Name:")); panel.add(tfName);
		panel.add(new JLabel("X:"));   panel.add(tfX);
		panel.add(new JLabel("Y:"));   panel.add(tfY);
		panel.add(new JLabel("Angle:")); panel.add(tfAngle);
		int result = JOptionPane.showConfirmDialog(f, panel,
			"Edit Position", JOptionPane.OK_CANCEL_OPTION,
			JOptionPane.PLAIN_MESSAGE);
		if (result == JOptionPane.OK_OPTION) {
			try {
				String newName = tfName.getText().trim();
				double newX = Double.parseDouble(tfX.getText().trim());
				double newY = Double.parseDouble(tfY.getText().trim());
				double newA = (Double.parseDouble(tfAngle.getText().trim())%360+360)%360;
				if (!newName.isEmpty()) {
					p.n=newName; p.x=newX; p.y=newY; p.a=newA;
					savePositions(); updateSavedList();
				}
			} catch (NumberFormatException ex) {
				JOptionPane.showMessageDialog(f,
					"Invalid number format!","Error",
					JOptionPane.ERROR_MESSAGE);
			}
		}
	}

	private JButton mkBtn(String t, Color c) {
		JButton b = new JButton(t);
		b.setFont(new Font("Arial",Font.BOLD,11));
		b.setBackground(c); b.setForeground(Color.WHITE);
		b.setFocusPainted(false); b.setBorderPainted(false);
		b.setPreferredSize(new Dimension(110,38));
		b.setMaximumSize(new Dimension(220,38));
		b.setCursor(new Cursor(Cursor.HAND_CURSOR));
		b.addMouseListener(new MouseAdapter() {
			public void mouseEntered(MouseEvent e) { b.setBackground(c.brighter()); }
			public void mouseExited(MouseEvent e)  { b.setBackground(c); }
		});
		return b;
	}

	private void drawCross(Graphics2D g) {
		double cx = getWidth()/2.0+offX;
		double cy = getHeight()/2.0+offY;
		g.setRenderingHint(RenderingHints.KEY_ANTIALIASING,
			RenderingHints.VALUE_ANTIALIAS_ON);
		g.setRenderingHint(RenderingHints.KEY_STROKE_CONTROL,
			RenderingHints.VALUE_STROKE_PURE);
		g.setStroke(new BasicStroke(lineThickness));
		g.setColor(crosshairColor);
		double r=Math.toRadians(a), sinR=Math.sin(r), cosR=Math.cos(r);
		double l = Math.max(getWidth(),getHeight())*3.0;
		g.draw(new Line2D.Double(
			cx-sinR*l, cy+cosR*l, cx+sinR*l, cy-cosR*l));
		g.draw(new Line2D.Double(
			cx-cosR*l, cy-sinR*l, cx+cosR*l, cy+sinR*l));
	}

	private void showSpectrumPicker(JButton swatch, JTextField hexField) {
		JDialog popup = new JDialog(f, false);
		popup.setUndecorated(true);
		popup.setAlwaysOnTop(true);

		// SV (saturation/value) canvas
		int CW = 200, CH = 150, HW = 200, HH = 16;
		float[] currentHSB = Color.RGBtoHSB(
			crosshairColor.getRed(), crosshairColor.getGreen(), crosshairColor.getBlue(), null);
		float[] pickerHue = { currentHSB[0] };
		float[] pickerSV  = { currentHSB[1], currentHSB[2] };

		JPanel svCanvas = new JPanel() {
			protected void paintComponent(Graphics g0) {
				super.paintComponent(g0);
				Graphics2D g = (Graphics2D) g0;
				// Draw sat/val gradient for current hue
				Color hueColor = Color.getHSBColor(pickerHue[0], 1f, 1f);
				// White -> hueColor (left to right)
				GradientPaint satGrad = new GradientPaint(0,0, Color.WHITE, CW,0, hueColor);
				g.setPaint(satGrad); g.fillRect(0,0,CW,CH);
				// Transparent -> black (top to bottom)
				GradientPaint valGrad = new GradientPaint(0,0, new Color(0,0,0,0), 0,CH, Color.BLACK);
				g.setPaint(valGrad); g.fillRect(0,0,CW,CH);
				// Cursor circle
				int cx = (int)(pickerSV[0] * CW);
				int cy = (int)((1f - pickerSV[1]) * CH);
				g.setColor(Color.WHITE);
				g.setStroke(new BasicStroke(1.5f));
				g.drawOval(cx-5, cy-5, 10, 10);
			}
		};
		svCanvas.setPreferredSize(new Dimension(CW, CH));
		svCanvas.setCursor(new Cursor(Cursor.CROSSHAIR_CURSOR));

		JPanel hueBar = new JPanel() {
			protected void paintComponent(Graphics g0) {
				super.paintComponent(g0);
				Graphics2D g = (Graphics2D) g0;
				// Rainbow gradient
				for (int x = 0; x < HW; x++) {
					g.setColor(Color.getHSBColor((float)x/HW, 1f, 1f));
					g.fillRect(x, 0, 1, HH);
				}
				// Cursor line
				int cx = (int)(pickerHue[0] * HW);
				g.setColor(Color.WHITE);
				g.setStroke(new BasicStroke(2f));
				g.drawLine(cx, 0, cx, HH);
			}
		};
		hueBar.setPreferredSize(new Dimension(HW, HH));
		hueBar.setCursor(new Cursor(Cursor.HAND_CURSOR));

		Runnable applySelection = () -> {
			Color c = Color.getHSBColor(pickerHue[0], pickerSV[0], pickerSV[1]);
			crosshairColor = c;
			swatch.setBackground(c);
			hexField.setText(colorToHex(c));
			repaint();
			svCanvas.repaint();
			hueBar.repaint();
		};

		MouseAdapter svHelper = new MouseAdapter() {
			void updateSV(MouseEvent e) {
				pickerSV[0] = Math.max(0f, Math.min(1f, (float)e.getX()/CW));
				pickerSV[1] = Math.max(0f, Math.min(1f, 1f - (float)e.getY()/CH));
				applySelection.run();
			}
			public void mousePressed(MouseEvent e) { updateSV(e); }
			public void mouseDragged(MouseEvent e) { updateSV(e); }
		};
		svCanvas.addMouseListener(svHelper);
		svCanvas.addMouseMotionListener(svHelper);

		MouseAdapter hueHelper = new MouseAdapter() {
			void updateHue(MouseEvent e) {
				pickerHue[0] = Math.max(0f, Math.min(1f, (float)e.getX()/HW));
				applySelection.run();
			}
			public void mousePressed(MouseEvent e) { updateHue(e); }
			public void mouseDragged(MouseEvent e) { updateHue(e); }
		};
		hueBar.addMouseListener(hueHelper);
		hueBar.addMouseMotionListener(hueHelper);

		JPanel container = new JPanel(new BorderLayout(0,4));
		container.setBackground(new Color(30,30,40));
		container.setBorder(BorderFactory.createLineBorder(new Color(80,80,100), 1));
		container.add(svCanvas, BorderLayout.CENTER);
		container.add(hueBar, BorderLayout.SOUTH);
		popup.add(container);
		popup.pack();

		// Position popup below the swatch button
		Point loc = swatch.getLocationOnScreen();
		popup.setLocation(loc.x, loc.y + swatch.getHeight() + 2);

		// Close when clicking outside
		popup.addWindowFocusListener(new WindowFocusListener() {
			public void windowLostFocus(WindowEvent e) { popup.dispose(); }
			public void windowGainedFocus(WindowEvent e) {}
		});
		popup.setVisible(true);
	}

	public static void main(String[] args) {
		SwingUtilities.invokeLater(CrosshairOverlayTransparent::new);
	}
}